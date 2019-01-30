/**
 * @file io.c
 * @brief 实现一个io缓冲区，支持buf_read,buf_write功能。
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-30
 * @copyright John Langford, license BSD
 */
#include "io.h"

/**
 * @brief 向缓冲区申请n字节的可读空间
 * 
 * @param i 缓冲区
 * @param pointer 可读空间的首地址（返回值）
 * @param n 申请可读空间大小
 * @return 实际可读空间大小
 * @see memmove() string.h
 * @note 根据缓冲区剩余空间，缓冲区全部空间分为三段情况考虑
 */
unsigned int buf_read(io_buf &i, char* &pointer, int n)
{
  // 缓冲区剩余空间足够，返回指向下n个字符的指针，$n<2^24$
  if (i.space.end + n <= i.space.end_array)
  {
    pointer = i.space.end;
    i.space.end += n;
    return n;
  }
  else if (n > (i.space.end_array - i.space.begin)) // 缓冲区完整空间不够，使用剩余缓冲区
  {
    pointer = i.space.end;
    int ret = i.space.end_array - i.space.end;
    i.space.end = i.space.end_array;
    return ret;
  }
  else // 缓冲区完整空间足够且剩余空间不够，清掉前面，重新读文件填充缓冲区。
  {
    int left = i.space.end_array - i.space.end;
    memmove(i.space.begin, i.space.end, left);
    i.fill(left);
    i.space.end = i.space.begin;
    return buf_read(i,pointer,n);
  }
}

/**
 * @brief 向缓冲区申请n大小的可写空间
 * 
 * @param o 缓冲区
 * @param pointer 可写空间首地址（返回值）
 * @param n 可写空间大小
 * @return 无
 * @see flush()
 * @note 考虑缓冲区不够n的情况
 */
void buf_write(io_buf &o, char* &pointer, int n)
{
  // 缓冲区空间足够，返回指向n字节空间的首地址
  if (o.space.end + n <= o.space.end_array)
  {
    pointer = o.space.end;
    o.space.end += n;
  }
  else // 缓冲区空间不够，将之前内容写入文件再申请
  {
    o.flush();
    buf_write (o, pointer,n);
  }
}
