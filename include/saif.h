#ifndef __SAIFCPP__H__
#define __SAIFCPP__H__
#include "json/value.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#define SAIF_ERR(expr, ...)                                                   \
  [[unlikely]] if(expr) {                                                     \
    std::cerr << fmt::format("[ERROR] @ line {} in {} ", __LINE__, __FILE__); \
    std::cerr << fmt::format(__VA_ARGS__) << std::endl;                       \
    exit(1);                                                                  \
  }

namespace saif {

enum state_t {
  SAIF_FILE,
  SAIF_FILE_VERSION,
  SAIF_DIRECTION,
  SAIF_DESIGN,
  SAIF_DATE,
  SAIF_VENDOR,
  SAIF_PROGRAM,
  SAIF_VERSION,
  SAIF_DIVIDER,
  SAIF_TIMESCALE,
  SAIF_DURATION,
  SAIF_INSTANCE,
  SAIF_NET,
  SAIF_PIN,
  SAIF_T0,
  SAIF_T1,
  SAIF_TX,
  SAIF_TZ,
  SAIF_TC,
  SAIF_IG,
  SAIF_TB
};

class SaifBase {
  public:
  std::string fullName;
  virtual state_t domain() = 0;
  virtual void getJson(Json::Value &) = 0;
};

class SaifPin : public SaifBase {
  public:
  uint64_t T0 = 0;
  uint64_t T1 = 0;
  uint64_t TX = 0;
  uint64_t TZ = 0;
  uint64_t TC = 0;
  uint64_t IG = 0;
  uint64_t TB = 0;
  SaifPin(const std::string &n) { fullName = n; }
  SaifPin() = delete;
  state_t domain() final { return SAIF_PIN; }
  void getJson(Json::Value &) final;
};

class SaifNet : public SaifBase {
  public:
  std::unordered_map<std::string, std::unique_ptr<SaifPin>> pins;
  SaifNet(const std::string &n) { fullName = n; }
  SaifNet() = delete;
  state_t domain() final { return SAIF_NET; }
  void getJson(Json::Value &) final;
};

class SaifInstance : public SaifBase {
  public:
  std::unique_ptr<SaifNet> net;
  std::unordered_map<std::string, std::unique_ptr<SaifInstance>> instances;
  SaifInstance(const std::string &n) { fullName = n; }
  SaifInstance() = delete;
  state_t domain() final { return SAIF_INSTANCE; }
  void getJson(Json::Value &) final;
};

class TokenQueue {
  private:
  boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>> q;
  std::ifstream ifs;
  std::string headCache;
  volatile bool headCacheValid = false;
  void load();
  volatile bool fileEnd = false;

  public:
  void start(const std::string &);
  void pop(std::string &);
  std::string front();
  bool empty();
};

class SaifDB {
  private:
  static std::unordered_map<std::string, SaifInstance *> globalInstanceMap;

  public:
  std::string version;
  std::string direction;
  std::string design;
  std::string date;
  std::string vendor;
  std::string programName;
  std::string toolVersion;
  std::string divider;
  uint64_t duration;
  std::string timescale;
  std::unique_ptr<SaifInstance> top;
  std::string top_name;
  Json::Value info;
  void parse(const std::string &);
  const Json::Value &getJson();
  static SaifInstance *findInstance(const std::string &);

  private:
  std::ifstream ifs;
  std::stack<SaifBase *> eStk;
  TokenQueue tokq;
  volatile bool infoGen = false;
  void saifParse();
};

}// namespace saif

#endif