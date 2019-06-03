/**
 * @file parse_args.cc
 * @brief 参数解析函数实现
 *
 * @author hjy
 * @version 2.3
 * @date 2019-01-30
 * @copyright John Langford, license BSD
 */
#include "parse_args.h"


namespace po = boost::program_options;
using namespace std;

/**
 * @brief 创建可写缓存文件
 * 
 * @param numbits 散列位数
 * @param create_cache 是否创建缓存文件
 * @param newname 缓存名称
 * @param cache 输入输出缓存
 * @return 空
 * @note 初始化时使用
 **/
void make_write_cache(size_t numbits, bool &create_cache, string &newname,
    io_buf &cache)
{
  cache.currentname = newname+string(".writing");

  cache.file = open(cache.currentname.c_str(), O_CREAT|O_WRONLY|O_LARGEFILE|O_TRUNC,0666);
  if (cache.file == -1) {
    cerr << "can't create cache file !" << endl;
    return;
  }
  char *p;
  buf_write(cache, p, sizeof(size_t));

  *(size_t *)p = numbits;

  cache.finalname = newname;
  create_cache = true;
  cout << "creating cache_file = " << newname << endl;
}

/**
 * @brief 创建可写缓存文件
 * 
 * @param vm boost库中的选项存储器
 * @param numbits 散列范围[0,2^{numbits}-1]
 * @param source 缓存名称
 * @param e 样本文件结构体
 * @return 空
 * @note 解析命令行中是否使用缓存文件时使用
 **/
void parse_cache(po::variables_map &vm, size_t numbits, string source,
    example_file &e)
{
  e.mask = (1 << numbits) - 1;
  // 命令包含cache或cache_file参数
  if (vm.count("cache") || vm.count("cache_file"))
  {
    string cache_string;
    if (vm.count("cache_file"))
      cache_string = vm["cache_file"].as< string >();
    else
      cache_string = source+string(".cache");
    e.cache.file = open(cache_string.c_str(), O_RDONLY|O_LARGEFILE);
    if (e.cache.file == -1)
      make_write_cache(numbits, e.write_cache, cache_string, e.cache);
    else {
      e.cache.fill(0);
      // 缓存文件存在但是和当前不一致
      if (inconsistent_cache(numbits, e.cache)) {
        close(e.cache.file);
        e.cache.space.erase();
        make_write_cache(numbits, e.write_cache, cache_string, e.cache);
      }
      else {
        cout << "using cache_file = " << cache_string << endl;
        e.write_cache = false;
      }
    }
  }
  else
  {
    cout << "using no cache" << endl;
    alloc(e.cache.space,0);
    e.cache.file=-1;
    e.write_cache = false;
  }
}

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
 * @todo SEG算法是嘛,有兴趣理解
 **/
void parse_args(int argc, char *argv[], float &eta, float &eta_decay_rate, float &initial_t,
    float &power_t, ofstream &predictions, ofstream &raw_predictions, bool &train,
    int &numthreads, int& passes, regressor &r, example_file &e,
    ofstream &final_regressor)
{
  // 定义支持的命令行参数
  po::options_description desc("Allowed options");
  desc.add_options()
    ("bit_precision,b", po::value<size_t>(&r.numbits)->default_value(18),"number of bits in the feature table")
    ("cache,c", "Use a cache.  The default is <data>.cache")
    ("cache_file", po::value< string >(), "The location of a cache_file.")
    ("data,d", po::value< string >()->default_value(""), "Example Set")
    ("decay_learning_rate",po::value<float>(&eta_decay_rate)->default_value(1/sqrt(2.)), "Set Decay Rate of Learning Rate")
    ("final_regressor,f", po::value< string >(), "Final regressor")
    ("help,h","Output Arguments")
    ("initial_regressor,i", po::value< vector<string> >(), "Initial regressor")
    ("initial_t", po::value<float>(&initial_t)->default_value(1.), "initial t value")
    ("power_t", po::value<float>(&power_t)->default_value(0.), "t power value")
    ("learning_rate,l", po::value<float>(&eta)->default_value(0.1), "Set Learning Rate")
    ("passes", po::value<int>(&passes)->default_value(1), "Number of Training Passes")
    ("predictions,p", po::value< string >(), "File to output predictions to")
    ("quadratic,q", po::value< vector<string> > (),"Create and use quadratic features")
    ("raw_predictions,r", po::value< string >(), "File to output unnormalized predictions to")
    ("seg,s", "Use SEG algorithm")
    ("testonly,t", "Ignore label information and just test")
    ("threads", po::value<int>(&numthreads)->default_value(1), "Number of threads")
    ;

  po::positional_options_description p;
  p.add("data", -1);

  // 解析命令行参数并存入选项存储器
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm); // 通知选项存储器更新变量结果

  if (vm.count("help") || argc == 1) {
    cout << "\n" << desc << "\n";
    exit(1);
  }

  if (r.numbits > 29) {
    cout << "The system limits at 29 bits of precision!\n" << endl;
    exit(1);
  }

  // 校验交叉验证指定集合格式
  if (vm.count("quadratic"))
  {
    r.pairs = vm["quadratic"].as< vector<string> >();
    cout << "creating quadratic features for pairs: ";
    for (vector<string>::iterator i = r.pairs.begin(); i != r.pairs.end();i++) {
      cout << *i << " ";
      if (i->length() > 2)
        cout << endl << "warning, ignoring characters after the 2nd.\n";
      if (i->length() < 2) {
        cout << endl << "error, quadratic features must involve two sets.\n";
        exit(0);
      }
    }
    cout << endl;
  }
  // 是否使用SEG算法
  r.seg = false;
  if (vm.count("seg"))
    r.seg = true;
  vector<string> regs;
  if (vm.count("initial_regressor"))
    regs = vm["initial_regressor"].as< vector<string> >();
  parse_regressor(regs, r);

  if (r.seg)
    cout << "SEG being used" << endl;

  parse_cache(vm, r.numbits, vm["data"].as<string>(), e);

  if (vm.count("data") && vm["data"].as<string> () != string(""))
  {
    string temp = vm["data"].as< string >();
    cout << "Reading from " << temp << endl;
    e.data = fopen64(temp.c_str(), "r");
    if (e.data == NULL)
    {
      cerr << "can't open " << temp << ", bailing!" << endl;
      exit(0);
    }
  }
  else
  {
    e.data = stdin;
    cout << "reading from stdin" << endl;
  }

  if (vm.count("final_regressor")) {
    cout << "final_regressor = " << vm["final_regressor"].as<string>() << endl;
    final_regressor.open(vm["final_regressor"].as<string>().c_str());
  }

  if (vm.count("predictions")) {
    cout << "predictions = " <<  vm["predictions"].as< string >() << endl;
    predictions.open(vm["predictions"].as< string >().c_str());
  }

  if (vm.count("raw_predictions")) {
    cout << "raw predictions = " <<  vm["raw_predictions"].as< string >() << endl;
    raw_predictions.open(vm["raw_predictions"].as< string >().c_str());
  }

  if (vm.count("testonly"))
  {
    cout << "only testing" << endl;
    train = false;
  }
  else 
    cout << "learning_rate set to " << eta << endl;
}
