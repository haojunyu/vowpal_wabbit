/**
 * @file parse_args.h
 * @brief 参数解析的声明
 *
 * @author hjy
 * @version 2.3
 * @date 2019-02-19
 * @copyright John Langford, license BSD
 */
// 宏定义之条件编译
#ifndef PA_H
#define PA_H

#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <math.h>
#include "fcntl.h"
#include "io.h"
#include "parse_regressor.h"
#include "parse_example.h"
using namespace std;
/**
 * @brief 解析命令行参数
 * 
 * @param argc 参数个数
 * @param argv 参数数组
 * @param eta 初始学习率
 * @param eta_decay_rate 学习率衰减因子
 * @param initial_t t初始值
 * @param power_t t power值
 * @param predictions 预测值存放文件
 * @param raw_predictions 原始概率值存放文件
 * @param train 是否训练
 * @param numthreads 训练线程数
 * @param passes 训练批次
 * @param r 回归模型参数
 * @param e 样本文件结构体
 * @param final_regressor 最终模型参数存放文件
 * @return 空
 * @todo SEG算法是嘛
 **/
void parse_args(int argc, char *argv[], float &eta, float &eta_decay_rate, float &initial_t,
    float &power_t, ofstream &predictions, ofstream &raw_predictions, bool &train, 
    int &numthreads, int& passes, regressor &r, example_file &e,
    ofstream &final_regressor);  //< 参数解析
#endif
