/**
 * @file parse_regressor.cc
 * @brief 回归模型结构体实现
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-30
 * @copyright John Langford, license BSD
 */
#include <fstream>
#include "parse_regressor.h"

/**
 * @brief 初始化回归模型结构体
 * 
 * @param r 回归模型结构体
 * @return void
 * @note 主要是weights（other_weights）的空间申请
 */
void initialize_regressor(regressor &r)
{
  r.length = 1 << r.numbits;
  if (r.seg)
  {
    r.weights = (weight *)malloc(r.length * sizeof(weight));
    weight* end = r.weights+r.length;
    for (weight *v = r.weights; v != end; v++)
    {
      *v = 1.;
    }
    r.other_weights = (weight *)malloc(r.length * sizeof(weight));
    end = r.other_weights + r.length;
    for (weight *v = r.other_weights; v != end; v++)
    {
      *v = 1.;
    }
  }else{
    // 非seg模式申请weights空间，并初始化为0
    r.weights = (weight *)calloc(r.length, sizeof(weight));
  }
}

/* 
   Read in regressors.  If multiple regressors are specified, do a weighted 
   average.  If none are specified, initialize according to global_seg & 
   numbits.
*/
/**
 * @brief 初始化回归模型参数，如果指定多个模型文件，则做加权平均，如果没有指定模型文件，则初始化回归模型
 * 
 * @param regressors  文件名称，文件保存的是初始化的回归模型特征值
 * @param r 回归模型参数
 * @return void
 * @todo seg的含义以及初始化问题
 * @note 训练时初始化回归模型，训练时从文件中读取回归模型参数
 */
void parse_regressor(vector<string> &regressors, regressor &r)
{
  bool initialized = false;

  for (size_t i = 0; i < regressors.size(); i++)
  {
    ifstream regressor(regressors[i].c_str());
    // 第一遍读取seg并赋值给r.seg
    bool seg;
    regressor.read((char *)&seg, sizeof(seg));
    if (!initialized)
    {
      r.seg = seg;
    }else{
      if (seg != r.seg)
      {
        cout << "can't combine regressors from seg and gd!" << endl;
        exit (1);
      }
    }

    // 第一遍读取local_numbits并赋值给r.numbits
    size_t local_numbits;
    regressor.read((char *)&local_numbits, sizeof(local_numbits));
    if (!initialized)
    {
      r.numbits = local_numbits;
    }else{
      if (local_numbits != r.numbits)
      {
        cout << "can't combine regressors with different feature number!" << endl;
        exit (1);
      }
    }

    // 第一遍读取local_pairs并赋值给r.pairs,并结束初始化
    int len;
    regressor.read((char *)&len, sizeof(len));
    vector<string> local_pairs;
    for (; len > 0; len--)
    {
      char pair[2];
      regressor.read(pair, sizeof(char)*2);
      string temp(pair, 2);
      local_pairs.push_back(temp);
    }
    if (!initialized)
    {
      r.pairs = local_pairs;
      initialize_regressor(r);
      initialized = true;
    }else{
      if (local_pairs != r.pairs)
      {
        cout << "can't combine regressors with different features!" << endl;
        for (size_t i = 0; i < local_pairs.size(); i++)
          cout << local_pairs[i] << " " << local_pairs[i].size() << " ";
        cout << endl;
        for (size_t i = 0; i < r.pairs.size(); i++)
          cout << r.pairs[i] << " " << r.pairs[i].size() << " ";
        cout << endl;
        exit (1);
      }
    }

    // TODO:seg与非seg的区别
    if (!seg)
    {
      while (regressor.good())
      {
        size_t hash;
        regressor.read((char *)&hash, sizeof(hash));
        weight w = 0.;
        regressor.read((char *)&w, sizeof(float));
        if (regressor.good())
          r.weights[hash] = r.weights[hash] + w;
      }
    }else{
      while (regressor.good())
      {
        size_t hash;
        regressor.read((char *)&hash, sizeof(hash));
        weight first = 0.;
        regressor.read((char *)&first, sizeof(float));
        weight second = 0.;
        regressor.read((char *)&second, sizeof(float));
        if (regressor.good()) {
          r.weights[hash] = first;
          r.other_weights[hash] = second;
        }
      }
    }
    regressor.close();
  }

  // 循环中initialized会被置为true
  if (!initialized)
  {
    initialize_regressor(r);
  }
}


/**
 * @brief 输出回归量结构体
 * 
 * @param o 输出流
 * @param r 回归量结构体
 * @return void
 * @note 按顺序输出seg,numbits,length,pairs,weight,other_weights
 */
void dump_regressor(ofstream &o, regressor &r)
{
  if (o.is_open())
  {
    o.write((char *)&r.seg, sizeof(r.seg));
    o.write((char *)&r.numbits, sizeof(r.numbits));
    int len = r.pairs.size();
    o.write((char *)&len, sizeof(len));
    for (vector<string>::iterator i = r.pairs.begin(); i != r.pairs.end();i++)
    {
      o << (*i)[0] << (*i)[1];
    }

    if (!r.seg) 
    {
      for(weight* v = r.weights; v != r.weights+r.length; v++)
      {
        if (*v != 0.)
        {
          size_t dist = v - r.weights;
          o.write((char *)&(dist), sizeof (dist));
          o.write((char *)v, sizeof (*v));
        }
      }
    }else{
      for(weight* v = r.weights; v != r.weights+r.length; v++)
      {
        if (*v != 1.)
        {
          size_t dist = v - r.weights;
          o.write((char *)&(dist), sizeof (dist));
          o.write((char *)v, sizeof (*v));
          o.write((char *)&r.other_weights[dist], sizeof (r.other_weights[dist]));
        }
      }
    }
  }

  if (r.seg)
  {
    free(r.other_weights);
  }

  free(r.weights);

  o.close();
}

