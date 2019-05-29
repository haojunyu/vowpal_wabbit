/**
 * @file stack.h
 * @brief 实现一个支持通用元素的栈，支持push,pop等功能。
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-29
 * @copyright John Langford, license BSD
 */
#ifndef __STACK_H
#define __STACK_H
#include <stdlib.h>
#include <string.h>
#include <algorithm>
using namespace std;


/**
 * @brief 支持通用元素的模板数组
 */
template<class T> class v_array{
  public:
    T* begin; ///< 数组数据头
    T* end;   ///< 数组数据尾
    T* end_array; ///< 数组空间尾部

    v_array() { begin= NULL; end = NULL; end_array=NULL;} ///< 构造函数
    T last() { return *(end-1);}  ///< 返回最后一个元素
    void decr() { end--;} ///< 删除最后一个元素
    T& operator[](unsigned int i) { return begin[i]; }  ///< 操作符[]重载
    unsigned int index(){return end-begin;} ///< 返回数组中元素个数
    void erase() { end = begin;}  ///< 清空数组元素
};

/**
 * @brief 将单个元素放到数组尾部
 * 
 * @param v 数组
 * @param new_ele 要推的元素
 * @return 无
 * @note 数据空间重新申请
 */
template<class T> void push(v_array<T>& v, const T &new_ele)
{
  if(v.end == v.end_array)
    {
      size_t old_length = v.end_array - v.begin;
      size_t new_length = 2 * old_length + 3;
      v.begin = (T *)realloc(v.begin,sizeof(T) * new_length);
      v.end = v.begin + old_length;
      v.end_array = v.begin + new_length;
    }
  *(v.end++) = new_ele;
}

/**
 * @brief 将多个元素放到数组尾部
 * 
 * @param v 数组
 * @param begin 多个元素数组
 * @param num 推的元素个数
 * @return 无
 * @see push()
 * @note 数据空间重新申请和内存拷贝
 */
template<class T> void push_many(v_array<T>& v, const T* begin, size_t num)
{
  if(v.end+num >= v.end_array)
  {
    size_t length = v.end - v.begin;
    size_t new_length = max(2 * (size_t)(v.end_array - v.begin) + 3,
		      v.end - v.begin + num);
    v.begin = (T *)realloc(v.begin,sizeof(T) * new_length);
    v.end = v.begin + length;
    v.end_array = v.begin + new_length;
  }
  memcpy(v.end, begin, num * sizeof(T));
  v.end += num;
}

/**
 * @brief 数组申请空间
 * 
 * @param v 数组
 * @param length 申请的数组空间
 * @return 无
 * @see realloc()
 */
template<class T> void alloc(v_array<T>& v, size_t length)
{
  v.begin = (T *)realloc(v.begin, sizeof(T) * length);
  v.end = v.begin;
  v.end_array = v.begin + length;
}

/**
 * @brief 弹出数组最后一个元素
 * 
 * @param stack 数组
 * @return v_array<T> 数组元素
 * @note 数组为空的情况
 */
template<class T> v_array<T> pop(v_array<v_array<T> > &stack)
{
  if (stack.end != stack.begin)
    return *(--stack.end);
  else
    return v_array<T>();
}

#endif  // __STACK_H
