/**
 * @file parse_example.cc
 * @brief 解析样本文件的实现 
 *
 * @author hjy 
 * @version 2.3 
 * @date 2019-02-25 
 * @copyright John Langford, license BSD 
 */
#include <vector>
#include <float.h>
#include <fcntl.h>
#include <pthread.h>
#include "parse_example.h"
#include "hash.h"

pthread_mutex_t input_mutex;  ///< 声明互斥变量input_mutex

/**
 * @brief 将字符串s按照分隔符delim来切分并push到ret中
 * 
 * @param delim 分隔符
 * @param s 字符串
 * @param ret 存放substring的数组
 * @return void
 * @note 
 */
void parse(char delim, substring s, v_array<substring> &ret)
{
  char *last = s.start;
  for (; s.start != s.end; s.start++) {
    if (*s.start == delim) {
      // 如果是连续delim，则不用push子串结构体
      if (s.start != last)
      {
        substring temp = {last,s.start};
        push(ret, temp);
      }
      last = s.start+1;
    }
  }
  // 最后一个子串
  if (s.start != last)
  {
    substring final = {last, s.start};
    push(ret, final);
  }
}


/**
 * @brief 将所有特征值对应的散列标记为false
 * 
 * @param in_already 存放features数组是否有值
 * @param features 存放所有特征的数组
 * @return void
 * @see mark()
 * @note in_already的长度为散列表的长度，features是所有特征值
 */
void unmark(bool* in_already, v_array<feature> &features)
{
  for (feature* j =features.begin; j != features.end; j++)
    in_already[j->weight_index] = false;
}


/**
 * @brief 将所有特征值对应的散列标记为ture
 * 
 * @param in_already 存放features数组是否有值
 * @param features 存放所有特征的数组
 * @return void
 * @see unmark()
 * @note in_already的长度为散列表的长度，features是所有特征值
 */
void mark(bool* in_already, v_array<feature> &features)
{
  for (feature* j =features.begin; j != features.end; j++)
    in_already[j->weight_index] = true;
}


/**
 * @brief 将字符串散列成长整型
 * 
 * @param s 字符串 
 * @param h 上一次散列值，随机种子
 * @return 散列值
 * @see hash() hash.h 
 * @note 内联函数
 */
inline size_t hashstring (substring s, unsigned long h)
{
  return hash((unsigned char *)s.start, s.end - s.start, h);
}

/**
 * @brief 将唯一的索引和权重添加成为特征，冲突的索引则丢弃
 * 
 * @param in_already 标记散列表是否已经被使用 
 * @param features  所有特征数组
 * @param v  要添加的特征权重
 * @param hashvalue  要添加的特征所对应的散列值
 * @return void
 * @note  内联函数，遇到冲突则丢弃特征
 */
inline void add_unique_index(bool *in_already, v_array<feature> &features, 
    float v, size_t hashvalue)
{
  size_t index = hashvalue;
  if (in_already[index] != true)
  {
    in_already[index] = true;
    feature f = {v,index};
    push(features, f);
  }
}


/**
 * @brief ???用来生成交叉特征的
 * 
 * @param in_already 标记散列表是否已经被使用 
 * @param pair_features  所有特征数组
 * @param first_part  交叉特征数组1
 * @param second_part  交叉特征数组2
 * @param mask  散列值掩码
 * @return void
 * @note  遍历，两两交叉，但是16381和(halfhash+index)&mask
 */
void new_quadratic(bool *in_already, v_array<feature> &pair_features, 
    const v_array<feature> &first_part, 
    const v_array<feature> &second_part, size_t mask)
{
  for (feature* i = first_part.begin; i != first_part.end; i++)
  {
    size_t halfhash = 16381 * i->weight_index;
    float i_value = i->x;
    for (feature* ele = second_part.begin; ele != second_part.end; ele++)
    {
      add_unique_index(in_already, pair_features, i_value * ele->x, 
        (halfhash + ele->weight_index) & mask);
    }
  }
}

size_t neg_1 = 1; ///< 表示特征权重为-1
size_t general = 2; ///< 表示特征权重不为1和-1之外的值

/**
 * @brief 从字符串中解码整数
 * 
 * @param p  待解码的字符串
 * @param i  返回的数值
 * @return   未处理的新字符串
 * @see run_len_encode()
 * @note  a,b,c表示7位二进制，1a1b0c -> cba
 */
char* run_len_decode(char *p, size_t& i)
{// read an int 7 bits at a time.
  size_t count = 0;
  while(*p & 128)
    i = i | ((*(p++) & 127) << 7*count++);
  i = i | (*(p++) << 7*count);
  return p;
}

pthread_mutex_t cache_lock;
size_t int_size = 5;  ///< 用5个字符来给int编码
size_t char_size = 2;

/**
 * @brief 从cache文件中读取一条样本特征
 * 
 * @param label  样本标签
 * @param weight  样本权重
 * @param cache  由文件*.cache形成的缓冲区
 * @param tag  
 * @param atomics  特征数组
 * @param indicies  
 * @return int  读取缓冲区中字节数
 * @see cache_features()
 * @note  样本=label,weight,num_indicies,tag_size,tag,[(index,storage,[(weight_index,x)...])...]
 */
int read_cached_features(float& label, float& weight, io_buf &cache,
    v_array<char> &tag, v_array<feature>* atomics,
    v_array<size_t> &indicies)
{
  char *p;
  pthread_mutex_lock(&cache_lock);
  size_t total = sizeof(label)+sizeof(weight)+int_size*2;
  size_t num_indicies = 0;
  size_t tag_size = 0;
  if (buf_read(cache, p, total) < total)
    goto badness;

  label = *(float *)p;
  p += sizeof(float);
  weight = *(float *)p;
  p += sizeof(float);
  p = run_len_decode(p, num_indicies);
  p = run_len_decode(p, tag_size);
  cache.set(p);
  if (buf_read(cache, p, tag_size) < tag_size)
    goto badness;
  push_many(tag, p, tag_size);

  for (;num_indicies > 0; num_indicies--)
  {
    size_t temp;
    // int_size的数据为index，通过run_len_decode解码
    if((temp = buf_read(cache,p,int_size + sizeof(size_t))) < char_size + sizeof(size_t)) {
      cerr << "truncated example! " << temp << " " << char_size +sizeof(size_t) << endl;
      goto badness;
    }

    size_t index = 0;
    p = run_len_decode(p, index);
    push(indicies, index);
    v_array<feature>* ours = atomics+index;
    size_t storage = *(size_t *)p;
    p += sizeof(size_t);
    cache.set(p);
    total += storage;
    if (buf_read(cache,p,storage) < storage) {
      cerr << "truncated example!" << endl;
      goto badness;
    }

    char *end = p+storage;

    size_t last = 0;

    for (;p!= end;)
    {
      feature f = {1., 0};
      p = run_len_decode(p,f.weight_index);
      if (f.weight_index & neg_1)
        f.x = -1.;
      else if (f.weight_index & general)
      {
        f.x = *(float *)p;
        p += sizeof(float);
      }
      // ??? last的作用
      f.weight_index = last + (f.weight_index >> 2);
      last = f.weight_index;
      push(*ours, f);
    }
    cache.set(p);
  }
  pthread_mutex_unlock(&cache_lock);

  return total;

badness:
  pthread_mutex_unlock(&cache_lock);
  return 0;
}

/**
 * @brief 对整数进行编码
 * 
 * @param p  字符串
 * @param i  要编码的数值
 * @return   写入了编码的字符串
 * @see run_len_decode()
 * @note  a,b,c表示7位二进制，cba -> 1a1b0c
 */
char* run_len_encode(char *p, size_t i)
{
  // 将一个整数安装7个字符来存储，最后的最高位置0，其他置1
  while (i >= 128)
  {
    *(p++) = (i & 127) | 128;
    i = i >> 7;
  }
  *(p++) = (i & 127);
  return p;
}

/**
 * @brief 按feature中weight_index排序的比较函数
 * 
 * @param first  第一个特征
 * @param second 第二个特征
 * @return   特征散列索引的大小
 * @see qsort()
 * @note  结合qsort使用
 */
int order_features(const void* first, const void* second)
{
  return ((feature*)first)->weight_index - ((feature*)second)->weight_index;
}

/**
 * @brief 缓存一条样本特征进cache文件
 * 
 * @param label  样本标签
 * @param weight  样本权重
 * @param cache  由文件*.cache形成的缓冲区
 * @param tag  
 * @param atomics  特征数组
 * @param indicies  
 * @return 空
 * @see read_cached_features()
 * @note  样本=label,weight,num_indicies,tag_size,tag,[(index,storage,[(weight_index,x)...])...]
 */
void cache_features(float label, float weight, io_buf &cache,
    v_array<char> &tag, v_array<feature>* atomics, 
    v_array<size_t> &indicies)
{
  char *p;

  pthread_mutex_lock(&cache_lock);

  buf_write(cache, p, sizeof(label)+sizeof(weight)+int_size*2+tag.index());

  *(float *)p = label;
  p += sizeof(label);
  *(float *)p = weight;
  p += sizeof(weight);

  p = run_len_encode(p, indicies.index());
  p = run_len_encode(p, tag.index());
  memcpy(p,tag.begin,tag.index());
  p += tag.index();
  cache.set(p);

  for (size_t* b = indicies.begin; b != indicies.end; b++)
  {
    size_t storage = atomics[*b].index() * int_size;
    feature* end = atomics[*b].end;
    for (feature* i = atomics[*b].begin; i != end; i++)
      // 1代表啥？？？-1代表没有值
      if (i->x != 1. && i->x != -1.)
        storage+=sizeof(float);

    buf_write(cache, p, int_size + storage + sizeof(size_t));
    p = run_len_encode(p, *b);
    char *storage_size_loc = p; ///< ???变量含义
    p += sizeof(size_t);

    qsort(atomics[*b].begin, atomics[*b].index(), sizeof(feature), order_features);
    size_t last = 0;

    for (feature* i = atomics[*b].begin; i != end; i++)
    {
      size_t diff = (i->weight_index - last) << 2;
      last = i->weight_index;
      if (i->x == 1.)
        p = run_len_encode(p, diff);
      else if (i->x == -1.)
        p = run_len_encode(p, diff | neg_1);
      else {
        p = run_len_encode(p, diff | general);
        *(float *)p = i->x;
        p += sizeof(float);
      }
    }
    cache.set(p);
    *(size_t*)storage_size_loc = p - storage_size_loc - sizeof(size_t);
  }
  pthread_mutex_unlock(&cache_lock);
}

/**
 * @brief 将字符子串结构体转换成float类型
 * 
 * @param s  字符子串结构体
 * @return   float类型数值
 * @see atof()
 */
inline float float_of_substring(substring s)
{
  return atof(string(s.start, s.end-s.start).c_str());
}

/**
 * @brief 将字符串s(name:value)中name和v提取出来
 *
 * @param s  字符子串结构体
 * @param name  存放name和value
 * @param v  返回的value值
 * @return  空
 */
void feature_value(substring &s, v_array<substring>& name, float &v)
{
  name.erase();
  parse(':', s, name);

  switch (name.index()) {
  case 0:
  case 1:
    break;
  case 2:
    v = float_of_substring(name[1]);
    break;
  default:
    cerr << "example with a wierd name.  What is ";
    cerr.write(s.start, s.end - s.start);
    cerr << "\n";
  }
}

/**
 * @brief 从文件data中解析一行样本特征置于label,weight,tag,stuff中
 *
 * @param stuff 特征存放结构
 * @param data  样本文件句柄
 * @param label 样本标签
 * @param weight 样本权重
 * @param tag 样本标识
 * @param mask 特征名散列掩码
 * @return 一行样本的字符数
 */
int read_features(thread_data &stuff, FILE* data, float &label,
    float &weight, v_array<char> &tag, size_t mask)
{
  pthread_mutex_lock(&input_mutex);
  int num_chars = getline (&stuff.line, &stuff.linesize, data);
  if (num_chars == -1) {
    pthread_mutex_unlock(&input_mutex);
    return num_chars;
  }

  substring page_offer = {stuff.line, stuff.line + num_chars-1};

  stuff.channels.erase();
  parse('|', page_offer, stuff.channels);

  // 解析标签，权重，样本标识
  stuff.words.erase();
  parse(' ',stuff.channels[0], stuff.words);
  switch(stuff.words.index()) {
  case 0:
    label = FLT_MAX;  ///< FLT_MAX 最大值浮点数
    break;
  case 1:
    label = float_of_substring(stuff.words[0]);
    weight = 1.;
    break;
  case 2:
    label = float_of_substring(stuff.words[0]);
    weight = float_of_substring(stuff.words[1]);
    break;
  case 3:
    label = float_of_substring(stuff.words[0]);
    weight = float_of_substring(stuff.words[1]);
    push_many(tag, stuff.words[2].start,
      stuff.words[2].end - stuff.words[2].start); ///< 样本标识
    break;
  default:
    cerr << "malformed example!\n";
  }

  // 处理多个Namespace
  for (substring* i = stuff.channels.begin+1; i != stuff.channels.end; i++) {
    substring channel = *i;

    stuff.words.erase();
    parse(' ',channel, stuff.words);
    if (stuff.words.begin == stuff.words.end)
      continue;

    float channel_v = 1.;
    feature_value(stuff.words[0], stuff.name, channel_v);

    // 以特征名的首字符构建特征数组，针对Namespace
    size_t index = 0;
    bool new_index = false;
    if (stuff.name.index() > 0) {
      index = (unsigned char)*(stuff.name[0].start);
      if (stuff.atomics[index].begin == stuff.atomics[index].end)
        new_index = true;
    }

    // 解析第二个之后的多个特征子串
    size_t channel_hash = hashstring(stuff.name[0], 0);
    for (substring* i = stuff.words.begin+1; i != stuff.words.end; i++) {
      float v = channel_v;
      feature_value(*i, stuff.name, v);

      size_t word_hash = (hashstring(stuff.name[0], channel_hash)) & mask;
      add_unique_index(stuff.in_already, stuff.atomics[index], v, word_hash);
    }

    // ?? 标记哪些index已经有特征
    if (new_index && stuff.atomics[index].begin != stuff.atomics[index].end)
      push(stuff.indicies, index);
  }

  pthread_mutex_unlock(&input_mutex);
  return num_chars;
}

/**
 * @brief 根据样本文件结构体example_source解析样本，并置于features中
 *
 * @param stuff 样本结构体
 * @param example_source 样本文件结构体
 * @param reg 回归模型结构体
 * @param features  特征数组
 * @param label  样本标签
 * @param weight  样本权重
 * @param tag  样本标识
 * @return  是否解析成功
 */
bool parse_example(thread_data &stuff, example_file &example_source,
    regressor &reg, v_array<feature> &features,
    float &label, float &weight, v_array<char> &tag)
{
  // 清空初始化
  features.erase();
  tag.erase();

  for (size_t* i = stuff.indicies.begin; i != stuff.indicies.end; i++)
    stuff.atomics[*i].erase();
  stuff.indicies.erase();

  // 没缓存文件或者需要写缓存文件，则尝试读样本文件
  if (example_source.cache.file == -1 || example_source.write_cache){
    if (read_features(stuff, example_source.data, label, weight, tag,
        example_source.mask) <= 0)
      return false;

    if (example_source.write_cache)
      cache_features(label,weight, example_source.cache, tag,
          stuff.atomics, stuff.indicies);
  }
  else // 使用缓存文件
  {
    if (read_cached_features(label, weight, example_source.cache, tag,
        stuff.atomics, stuff.indicies) <= 0)
      return false;
  }

  // 汇总所有特征
  for (size_t* i = stuff.indicies.begin; i != stuff.indicies.end; i++)
    push_many(features, stuff.atomics[*i].begin, stuff.atomics[*i].index());

  // 有缓存文件但不需要写缓存文件，则标记所有特征???
  if (example_source.cache.file != -1 && !example_source.write_cache
      /*&& reg.pairs.size() > 0*/)
    mark(stuff.in_already, features);

  // 根据reg.pairs来生成交叉特征
  for (vector<string>::iterator i = reg.pairs.begin(); i != reg.pairs.end();i++)
    new_quadratic(stuff.in_already, features, stuff.atomics[(int)(*i)[0]],
        stuff.atomics[(int)(*i)[1]], example_source.mask);

  // 添加常量特征
  add_unique_index(stuff.in_already, features, 1, 0);//a constant features

  // 取消所有标记的特征???
  unmark(stuff.in_already, features);

  return true;
}

/**
 * @brief 检测缓存文件是否一致
 *
 * @param numbits ???
 * @param cache  输入输出缓存
 * @return  缓冲区是否一致
 */
bool inconsistent_cache(size_t numbits, io_buf &cache)
{
  size_t total = sizeof(numbits);
  char *p;
  if (buf_read(cache, p, total) < total)
    return true;

  size_t cache_numbits = *(size_t *)p;
  if (cache_numbits != numbits)
    return true;

  return false;
}

/**
 * @brief 重置样本文件结构体状态
 *
 * @param numbits ???
 * @param source 样本文件结构体
 * @return  空
 * @note  初始和生成缓存文件后会执行
 */
void reset(size_t numbits, example_file &source)
{
  if (source.write_cache)
  {
    source.cache.flush();
    source.write_cache = false;
    close(source.cache.file);
    rename(source.cache.currentname.c_str(), source.cache.finalname.c_str());
    source.cache.file = open(source.cache.finalname.c_str(), O_RDONLY|O_LARGEFILE);
  }
  if (source.cache.file != -1)
  {
    lseek(source.cache.file, 0, SEEK_SET); ///< 置文件读写位置为0
    alloc(source.cache.space, 1 << 16); ///< 初始申请2^16Byte空间
    source.cache.fill(0);
    // 核实缓存中的numbit和运行中的numbit是否一致
    if (inconsistent_cache(numbits, source.cache)) {
      cout << "argh, a bug in caching of some sort!  Exitting\n" ;
      exit(1);
    }
  }
  else
    fseek(source.data, 0, SEEK_SET);
}

/**
 * @brief 清理程序空间
 *
 * @param source 样本文件结构体
 * @return  空
 * @note  程序结束后用来清理空间
 */
void finalize_source(example_file &source)
{
  fclose(source.data);
  if (source.cache.file != -1) {
    close(source.cache.file);  
    free(source.cache.space.begin);
  }
}
