/**
 * @file parse_example.h
 * @brief 的声明 
 *
 * @author hjy
 * @version 2.3
 * @date 2019-02-23
 * @copyright John Langford, license BSD
 */

#ifndef PE_H
#define PE_H
#include "stack.h"
#include "io.h"
#include "parse_regressor.h"

using namespace std;

/**
 * @brief 特征结构体
 */
struct feature {
  float x;  ///< 权重数值
  size_t weight_index;  ///< 特征的散列索引
};

/**
 * @brief 字符子串结构体
 */
struct substring {
  char *start;  ///< 子串开头
  char *end;  ///< 子串结尾
};

/**
 * @brief 样本结构体
 */
struct thread_data {
  size_t linesize;  ///< 每行样本长度
  char *line; ///< 样本字符串

  v_array<substring> channels;  ///< 以'|'分割的多个字符子串
  v_array<substring> words; ///< 以' '分割每个channel得到的字符串
  v_array<substring> name; ///< 以':'分割每个feature:value得到的字符串

  v_array<size_t> indicies; ///< 特征名首字母，不同的Namespace
  v_array<feature> atomics[256]; ///< 特征列表数组
  bool *in_already; ///< 散列表标记
};

/**
 * @brief 样本文件结构体
 */
struct example_file {
  FILE* data; ///< 文件句柄
  io_buf cache; ///< 输入输出缓存
  bool write_cache; ///< 是否写缓存文件
  size_t mask;
};

/**
 * @brief 根据样本文件结构体解析样本
 *
 * @param stuff 特征存放结构
 * @param example_source 
 * @param reg 
 * @param features  特征数组
 * @param label  样本标签
 * @param weight  样本权重
 * @param tag  样本标识
 * @return  是否解析成功
 */
bool parse_example(thread_data &stuff, example_file &example_source, 
    regressor &reg, v_array<feature> &features,
    float &label, float &weight, v_array<char> &tag);

/**
 * @brief 检测缓存文件是否一致
 *
 * @param numbits ???
 * @param cache  输入输出缓存
 * @return  缓冲区是否一致
 */
bool inconsistent_cache(size_t numbits, io_buf &cache);

/**
 * @brief 重置样本文件结构体状态
 *
 * @param numbits ???
 * @param source 样本文件结构体
 * @return  空
 * @note  初始和生成缓存文件后会执行
 */
void reset(size_t numbits, example_file &source);

/**
 * @brief 清理程序空间
 *
 * @param source 样本文件结构体
 * @return  空
 * @note  程序结束后用来清理空间
 */
void finalize_source(example_file &source);

#endif
