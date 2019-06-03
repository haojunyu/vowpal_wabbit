/**
 * @file io.h
 * @brief 声明一个io缓冲区，支持buf_read,buf_write功能。
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-30
 * @copyright John Langford, license BSD
 */
#ifndef IO_H__
#define IO_H__

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "stack.h"

using namespace std;

/**
 * @brief 利用数组实现一个io缓冲区
 * @class v_array stack.h
 */
class io_buf {
  public:
    v_array<char > space;   ///< 内存缓冲区
    int file;   ///< 文件句柄
    string currentname;   ///< 
    string finalname;   ///< 
  
    io_buf() {alloc(space, 1 << 16); }  ///< 构造函数，申请$2^16$字符空间
    void set(char *p){space.end = p;}  ///< 设置缓冲区末尾，常用来删除缓冲区内容
    /**
     * @brief 从文件file中读取数据填满缓冲区
     * 
     * @param n 当前缓冲区元素个数
     * @return 无
     * @see read() unistd.h
     * @note alloc()是v_array的成员函数
     * @todo 没明白read后为什么还要alloc: 个人理解是为了置end=begin。该函数一般在初始化时使用
     */
    void fill(int n) {
      alloc(space,n + read(file, space.begin+n, space.end_array - space.begin -n));
    }

    /**
     * @brief 从缓冲区中的数据写到文件中
     * 
     * @param 无
     * @return 无
     * @see write() unistd.h
     * @see fsync() unistd.h
     * @note fsync会同步数据和文件属性
     */
    void flush() { 
      if (write(file, space.begin, space.index()) != (int) space.index()){
        cerr << "error, failed to write to cache\n";
      }
      space.end = space.begin; 
      fsync(file); 
    }
};

void buf_write(io_buf &o, char* &pointer, int n); ///< 向缓冲区写数据
unsigned int buf_read(io_buf &i, char* &pointer, int n);  ///< 从缓冲区读数据
#endif
