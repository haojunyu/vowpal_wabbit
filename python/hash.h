/**
 * @file hash.h
 * @brief 声明类型别名和hash函数。
 *
 * @author hjy
 * @version 2.3
 * @date 2019-02-14
 * @copyright John Langford, license BSD
 */
typedef  unsigned long  int  ub4; ///< 声明ub4
typedef  unsigned       char ub1; ///< 声明ub1

ub4 hash(ub1* k,ub4 length,ub4 initval);  ///< 将字符串hash成数值
