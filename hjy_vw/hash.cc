/**
 * @file hash.cc
 * @brief 实现hash函数。
 *
 * @author hjy
 * @version 2.3
 * @date 2019-02-14
 * @copyright John Langford, license BSD
 */
#include "hash.h"

#define hashsize(n) ((ub4)1<<(n)) ///< 宏定义：向左位移n位，n<32
#define hashmask(n) (hashsize(n)-1) ///< 宏定义：获得n位的位掩码


/* mix -- mix 3 32-bit values reversibly.
For every delta with one or two bits set, and the deltas of all three
  high bits or all three low bits, whether the original value of a,b,c
  is almost all zero or is uniformly distributed,
* If mix() is run forward or backward, at least 32 bits in a,b,c
  have at least 1/4 probability of changing.
* If mix() is run forward, every bit of c will change between 1/3 and
  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
mix() takes 36 machine instructions, but only 18 cycles on a superscalar
  machine (like a Pentium or a Sparc).  No faster mixer seems to work,
  that's the result of my brute-force search.  There were about 2^68
  hashes to choose from.  I only tested about a billion of those.
*/
/**
 * @brief TODO
 * 
 * @param a/b/c TODO
 * @see http://burtleburtle.net/bob/hash/doobs.html
 * @note 定义宏函数mix
 */
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}




/* hash() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  len     : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.
The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.
If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
By Bob Jenkins, 1996.  bob_jenkins@compuserve.com.  You may use this
code any way you wish, private, educational, or commercial.  It's free.
See http://ourworld.compuserve.com/homepages/bob_jenkins/evahash.htm
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
*/

/**
 * @brief 将可变长的秘钥散列成32位值
 * 
 * @param k 可变长的秘钥（文本）
 * @param length 秘钥长度，单位为字节
 * @param initval 上一次散列值（32位值）
 * @return 散列值（32bit）
 * @see http://ourworld.compuserve.com/homepages/bob_jenkins/evahash.htm
 * @note 秘钥的每位都会影响最终的散列值，hash表的长度最好是2的指数，对于
 * 不足32bit的使用位掩码，即`h = (h & hashmask(10));`
 */
ub4 hash(ub1* k,ub4 length,ub4 initval)
{
  register ub4 a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
  c = initval;         /* the previous hash value */

  // 处理大部分秘钥,三个四字符mix
  while (len >= 12)
  {
    a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
    b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
    c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
    mix(a,b,c);
    k += 12; len -= 12;
  }
  // 处理最后的字节（最多11个）
  c += length;
  switch(len)              // 注意len=11时会执行所有case
  {
    case 11: c+=((ub4)k[10]<<24);
    case 10: c+=((ub4)k[9]<<16);
    case 9 : c+=((ub4)k[8]<<8);
    case 8 : b+=((ub4)k[7]<<24);
    case 7 : b+=((ub4)k[6]<<16);
    case 6 : b+=((ub4)k[5]<<8);
    case 5 : b+=k[4];
    case 4 : a+=((ub4)k[3]<<24);
    case 3 : a+=((ub4)k[2]<<16);
    case 2 : a+=((ub4)k[1]<<8);
    case 1 : a+=k[0];
    // 注意len=0时不执行任何case
  }
  mix(a,b,c);

  return c;
}
