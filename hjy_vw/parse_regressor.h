/**
 * @file parse_regressor.h
 * @brief 解析回归量函数的声明 
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-30
 * @copyright John Langford, license BSD
 */
#ifndef PR_H
#define PR_H

#include <iostream>
#include <vector>

using namespace std;

typedef float weight; ///< 定义weight为float类型别名

/**
 * @brief 回归量结构体
 */
struct regressor {
  weight* weights;
  weight* other_weights;
  size_t numbits; ///< 散列表位数
  size_t length; ///< 散列表长度=2^{numbits}
  vector<string> pairs; ///< 交叉特征数组
  bool seg;
};

void parse_regressor(vector<string> &regressors, regressor &r);  ///< 解析regressors，构造回归量结构体

void dump_regressor(ofstream &o, regressor &r); ///< 输出回归量结构体

#endif
