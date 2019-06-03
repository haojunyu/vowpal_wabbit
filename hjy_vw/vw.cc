/**
 * @file vw.cc
 * @brief vowpal wabbit实现
 *
 * @author hjy
 * @version 2.3
 * @date 2019-05-27
 * @copyright John Langford, license BSD
 */

#include <math.h>
#include <iostream>
#include <fstream>
#include <float.h>
#include <pthread.h>
#include "parse_regressor.h"
#include "parse_example.h"
#include "parse_args.h"

using namespace std;

pthread_mutex_t weight_lock;  ///< 更新权重锁
float eta = 0.1;  ///< 学习率 
float t = 1.; ///< todo 
float power_t = 0.; ///< todo
ofstream predictions; ///< 预测值存放文件
ofstream raw_predictions; ///< 原始概率值存放文件
bool training = true; /// 是否训练

float dump_interval = exp(1.);  ///< dump间隔
double sum_loss = 0.0;  ///< 总损失值
double sum_loss_since_last_dump = 0.0;  ///< 上次dump之后的总损失值
long long int example_number = 0; ///< 样本总数目
double weighted_examples = 0.;  ///< 样本总权重
double old_weighted_examples = 0.;  ///< 上次dump时的样本总权重
double weighted_labels = 0.;  ///< 标签加权和sum(label*weight)

regressor regressor;
example_file source;
ofstream final_regressor;

/**
 * @brief 处理预测值到区间[0,1]
 * 
 * @param ret 预测值
 * @return 处理后的预测值
 */
inline float final_prediction(float ret) {
  if (isnan(ret))
    return 0.5;
  if ( ret > 1.0 )
    return 1.0;
  if (ret < 0.0)
    return 0.0;
  return ret;
}

/**
 * @brief 特征权重求和
 * 
 * @param features 特征数组
 * @param w 模型权重数组
 * @return 特征权重之和
 * @todo 为啥不是模型权重*特征值
 * @note seg算法专用，4.0舍弃
 */
float vector_sum(const v_array<feature> &features, weight* w)
{
  float sum = 0.0;
  for (feature* ele = features.begin; ele != features.end; ele++)
    sum += w[ele->weight_index];

  return sum;
}

/**
 * @brief seg算法预测的值
 * 
 * @param features 特征数组
 * @param neg_weight_sum 负权重之和
 * @param pos_weight_sum 正权重之和
 * @param raw_prediction 预测概率值
 * @return seg算法预测的值
 * @todo raw_prediction为啥取pos_weight_sum
 */
float seg_predict(v_array<feature> &features,
    float &neg_weight_sum, float &pos_weight_sum,
    float &raw_prediction)
{
  pos_weight_sum = vector_sum(features, regressor.weights);
  neg_weight_sum = vector_sum(features, regressor.other_weights);

  raw_prediction = pos_weight_sum;

  if (fpclassify(neg_weight_sum) == FP_ZERO)
    return 0.5;
  else
    return final_prediction(pos_weight_sum / (neg_weight_sum+pos_weight_sum));
}

/**
 * @brief
 *
 * @param features 特征数组
 * @param w 权重数组
 * @param update 
 * @return 
 * @todo 尚未理解SEG算法
 * @note SEG算法专用,4.0放弃
 */
void vector_multiply(const v_array<feature> &features, weight *w,
    const float update)
{
  for (feature* ele = features.begin; ele != features.end; ele++)
    w[ele->weight_index] *= update;
}

/**
 * @brief
 *
 * @param features 特征数组
 * @param neg_weight_update 
 * @param pos_weight_update 
 * @return 
 * @todo 尚未理解SEG算法
 * @note SEG算法专用，4.0放弃
 */
void seg_train(v_array<feature> &features,
    float neg_weight_update, float pos_weight_update)
{
  vector_multiply(features, regressor.weights, pos_weight_update);
  vector_multiply(features, regressor.other_weights, neg_weight_update);
}

/**
 * @brief 计算线性代价函数\f$h_w(x)=\sum_{i=0}^{i<N} x_i*w_i\f$
 *
 * @param features 特征数组
 * @param norm 归一化数值
 * @param raw_prediction 预测概率值
 * @return \f$final\_prediction(h_w(x)/\sqrt{N})\f$
 */
float predict(const v_array<feature> &features, float &norm, float & raw_prediction)
{
  float prediction = 0.0;
  weight *weights = regressor.weights;
  for (feature* j = features.begin; j != features.end; j++)
    prediction += weights[j->weight_index] * j->x;

  if (features.end - features.begin > 0)
    norm = 1. / sqrtf(features.end - features.begin);
  else
    norm = 1.;

  raw_prediction = prediction;
  prediction *= norm;

  return final_prediction(prediction);
}

/**
 * @brief 训练回归模型：更新权重
 *
 * @param features 特征数组
 * @param update 更新系数
 * @return \f$w_{j}=w_{j}-\alpha \frac{h_{w}(x)-y_{i}}{\sqrt{n_{w_{j}}}+1}*sample\_weight\f$
 */
void train(const v_array<feature> &features, float update)
{
  if (fabs(update) > 0.)
  {
    weight* weights = regressor.weights;
    for (feature* j = features.begin; j != features.end; j++)
      weights[j->weight_index] += update * j->x;
  }
}

/**
 * @brief 单线程更新模型参数
 *
 * @param in 输入
 * @return 无
 * @todo t为啥要累计weight
 */
void* go(void *in)
{
  thread_data td;
  td.line = NULL;
  td.linesize = 0;
  td.in_already = (bool*)calloc(regressor.length, sizeof(bool));

  v_array<feature> features;
  v_array<char> tag;
  float label;
  float weight;

  while ( parse_example(td, source, regressor, features, label, weight, tag))
  {
    if (features.end == features.begin)
    {
      cout << "You have an example with no features.  Skipping.\n";
      continue;
    }

    pthread_mutex_lock(&weight_lock);

    float norm = 0.;
    float neg_weight_sum = 0.;
    float prediction;
    float raw_prediction;
    if (regressor.seg)
      prediction = seg_predict(features,neg_weight_sum,norm,raw_prediction);
    else
      prediction = predict(features,norm,raw_prediction);

    example_number++;
    weighted_examples += weight;
    weighted_labels += label * weight;

    // 输出预测值到预测文件中prediction tag
    predictions << prediction;
    if (tag.begin != tag.end){
      predictions << " ";
      predictions.write(tag.begin, tag.index());
    }
    predictions << endl;

    // 输出预测概率值到预测概率文件中raw_prediction tag
    raw_predictions << raw_prediction;
    if (tag.begin != tag.end) {
      raw_predictions << " ";
      raw_predictions.write(tag.begin,tag.index());
    }
    raw_predictions << endl;

    if (label != FLT_MAX)
    {
      float example_loss = (prediction - label) * (prediction - label);
      example_loss *= weight;

      // t为啥加weight
      t += weight;
      sum_loss = sum_loss + example_loss;
      sum_loss_since_last_dump += example_loss;
      // 中间输出
      if (weighted_examples > dump_interval)
      {
        cout.precision(4);
        cout << sum_loss/weighted_examples << "\t"
            << sum_loss_since_last_dump / (weighted_examples - old_weighted_examples) << "\t"
            << example_number << "\t";
        cout.precision(2);
        cout << weighted_examples << "\t";
        cout.precision(4);
        cout << label << "\t" << prediction << "\t"
            << features.index() << "\t" << endl;

        sum_loss_since_last_dump = 0.0;
        old_weighted_examples = weighted_examples;
        dump_interval *= 2;
      }
    }
    // 每个样本去训练模型
    if (label != FLT_MAX && training)
    {
      float eta_round = eta / pow(t, power_t);
      if (regressor.seg) {
        float update = expf(2.0 * eta_round * (label - prediction) * weight);
        float neg_update;
        if (fpclassify(norm) == FP_ZERO && fpclassify(neg_weight_sum) == FP_ZERO)
          neg_update = 1.;
        else
          neg_update = (norm + neg_weight_sum) / (norm * update + neg_weight_sum);
        if (isnan(neg_update) || isnan(update*neg_update)) 
        {
          cout << "argh!, a nan! neg_update = " << neg_update << endl;
          cout << "label = " << label << endl;
          cout << "prediction = " << prediction << endl;
          cout << "norm = " << norm << endl;
          cout << "neg_weight_sum = " << neg_weight_sum << endl;
          cout << "feature number = " << features.index() << endl;
          cout << "update = " << update << endl;
          cout << "example_number = " << example_number << endl;
          for (feature* ele = features.begin; ele != features.end; ele++) 
            cout << ele->weight_index << "\t" << regressor.weights[ele->weight_index] << " " 
                << regressor.other_weights[ele->weight_index] << endl;

        }
        seg_train(features, neg_update, update * neg_update);

        float new_neg = 0.;
        float new_pos = 0.;
        float new_raw = 0.;
        seg_predict(features, new_neg, new_pos, new_raw);
        if (fabs(new_pos + new_neg - norm - neg_weight_sum) > (norm+neg_weight_sum) * 0.001) {
          cout.precision(20);
          cout << "Update isn't preserving weight!\n";
          cout << new_pos + new_neg << "\t" << norm + neg_weight_sum << endl;
          cout << "label = " << label << endl;
          cout << "prediction = " << prediction << endl;
          cout << "norm = " << norm << endl;
          cout << "neg_weight_sum = " << neg_weight_sum << endl;
          cout << "feature number = " << features.index() << endl;
          cout << "update = " << update << endl;
          cout << "example_number = " << example_number << endl;
          cout << "Features = " << endl;
          for (feature* ele = features.begin; ele != features.end; ele++) 
            cout << ele->weight_index << "\t" << regressor.weights[ele->weight_index] << "\t" 
                << regressor.other_weights[ele->weight_index] << endl;
        }
      }
      else
        train(features, eta_round * (label - prediction) * 2. * norm * weight);
    }
    pthread_mutex_unlock(&weight_lock);
  }

  alloc(features,0);
  alloc(tag,0);
  free(td.in_already);
  free(td.line);
  free(td.channels.begin);
  free(td.words.begin);
  free(td.name.begin);
  alloc(td.indicies,0);
  for (int i = 0; i < 256; i++)
    free(td.atomics[i].begin);

  return NULL;
}

/**
 * @brief 主函数
 *
 * @param argc 参数个数
 * @param argv 参数数组
 * @return
 */
int main(int argc, char *argv[])
{
  int numthreads;
  int numpasses;
  float eta_decay;
  parse_args(argc, argv, eta, eta_decay, t, power_t, predictions,
      raw_predictions, training, numthreads, numpasses, regressor, source,
      final_regressor);

  cout << "average\tsince\texample\texample\tcurrent\tcurrent\tcurrent" << endl;
  cout << "loss\tlast\tcounter\tweight\tlabel\tpredict\tfeatures" << endl;
  cout.precision(4);

  // 多线程训练模型
  for (; numpasses > 0; numpasses--) {
    pthread_t threads[numthreads];
    for (int i = 0; i < numthreads; i++)
      pthread_create(&threads[i], NULL, go, NULL);

    for (int i = 0; i < numthreads; i++) 
      pthread_join(threads[i], NULL);

    eta *= eta_decay;
    reset(regressor.numbits, source);
  }

  dump_regressor(final_regressor, regressor);
  finalize_source(source);
  float best_constant = weighted_labels / weighted_examples;
  float constant_loss = (best_constant*(1.0 - best_constant)*(1.0 - best_constant)
      + (1.0 - best_constant)*best_constant*best_constant);

  cout << endl << "finished run";
  cout << endl << "number of examples = " << example_number;
  cout << endl << "weighted_examples = " << weighted_examples;
  cout << endl << "weighted_labels = " << weighted_labels;
  cout << endl << "average_loss = " << sum_loss / weighted_examples;
  cout << endl << "best constant's loss = " << constant_loss;
  cout << endl;

  return 0;
}
