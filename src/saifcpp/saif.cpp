#include "saif.h"
#include "fmt/core.h"
#include "json/value.h"
#include <boost/token_functions.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using fmt::format;

namespace saif {

const std::unordered_map<std::string, state_t> lexMap{
  {"/**", SAIF_COMMENT},
  {"SAIFILE", SAIF_FILE},
  {"SAIFVERSION", SAIF_FILE_VERSION},
  {"DIRECTION", SAIF_DIRECTION},
  {"DESIGN", SAIF_DESIGN},
  {"DATE", SAIF_DATE},
  {"VENDOR", SAIF_VENDOR},
  {"PROGRAM_NAME", SAIF_PROGRAM},
  {"VERSION", SAIF_VERSION},
  {"DIVIDER", SAIF_DIVIDER},
  {"TIMESCALE", SAIF_TIMESCALE},
  {"DURATION", SAIF_DURATION},
  {"INSTANCE", SAIF_INSTANCE},
  {"NET", SAIF_NET},
  {"PORT", SAIF_NET},
  {"T0", SAIF_T0},
  {"T1", SAIF_T1},
  {"TX", SAIF_TX},
  {"TZ", SAIF_TZ},
  {"TC", SAIF_TC},
  {"IG", SAIF_IG},
  {"TB", SAIF_TB}};

void domainWalker(TokenQueue &tokq) {
}

void nameNormalize(std::string &str, bool keep) {
  if(keep) return;
  std::size_t found = str.find("\\");
  while(found != std::string::npos) {
    str.erase(found, 1);
    found = str.find("\\");
  }
}

string getVal(TokenQueue &q) {
  vector<string> tov;
  string buf = "";
  string res = "";
  bool leave = false;
  while(!leave) {
    q.pop(buf);
    if(buf == ")")
      leave = true;
    else if(res.empty())
      res = buf;
    else
      res = res + ' ' + buf;
  }
  return res;
}

void SaifDB::parse(const string &file, bool dontTouchInstName) {
  keep = dontTouchInstName;
  tokq.start(file);
  saifParse();
}

void SaifDB::saifParse() {
  string buf = "";
  string key = "";
  tokq.pop(buf);
  tokq.pop(key);
  SAIF_ERR(buf != "(", "Illegal start of domain {}", key);
  state_t state;
  if(!eStk.empty() && eStk.top()->domain() == SAIF_NET) {
    state = SAIF_PIN;
  } else if(lexMap.contains(key)) {
    state = lexMap.at(key);
  } else {
    SAIF_ERR(true, "Illegal declaration {}", key);
  }
  switch(state) {
  case SAIF_COMMENT: {
    do {
      tokq.pop(buf);
    } while(buf != "**/");
    break;
  }
  case SAIF_FILE: {
    saifParse();
    tokq.pop(buf);
    SAIF_ERR(buf != ")", "Illegal end of file");
    SAIF_ERR(!eStk.empty(), "Entities stack not clear when meet the end");
    break;
  }
  case SAIF_FILE_VERSION: {
    version = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_DIRECTION: {
    direction = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_DESIGN: {
    design = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_DATE: {
    date = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_VENDOR: {
    vendor = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_PROGRAM: {
    programName = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_VERSION: {
    toolVersion = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_DIVIDER: {
    divider = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_TIMESCALE: {
    timescale = getVal(tokq);
    saifParse();
    break;
  }
  case SAIF_DURATION: {
    duration = std::stod(getVal(tokq));
    saifParse();
    break;
  }
  case SAIF_INSTANCE: {
    tokq.pop(buf);
    nameNormalize(buf, keep);
    auto child = make_unique<SaifInstance>(buf);
    [[unlikely]]
    if(eStk.empty()) {
      eStk.push(child.get());
      top = std::move(child);
    } else {
      SaifBase *stkTop = eStk.top();
      SAIF_ERR(stkTop->domain() != SAIF_INSTANCE, "Parent of instance {} is wired", buf);
      SaifInstance *parent = dynamic_cast<SaifInstance *>(stkTop);
      SAIF_ERR(parent->instances.contains(buf), "INSTANCE {} has already been parsed", buf);
      eStk.push(child.get());
      child->fullName = parent->fullName + '.' + buf;
      globalInstanceMap[child->fullName] = child.get();
      parent->instances[buf] = std::move(child);
    }
#ifdef LOG_VERBOSE
    cout << format("instance {}: ", buf) << endl;
#endif
    while(tokq.front() != ")") saifParse();
    eStk.pop();
    tokq.pop(buf);
    break;
  }
  case SAIF_NET: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_INSTANCE;
    SAIF_ERR(failed, "Illegal NET/PORT declaration!");
    SaifInstance *parent = dynamic_cast<SaifInstance *>(eStk.top());
    auto child = make_unique<SaifNet>(parent->fullName);
    eStk.push(child.get());
    parent->net = std::move(child);
    while(tokq.front() != ")") saifParse();
    eStk.pop();
    tokq.pop(buf);
    break;
  }
  case SAIF_PIN: {
    nameNormalize(key, keep);
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_NET;
    SAIF_ERR(failed, "Illegal PIN declaration!");
    SaifNet *parent = dynamic_cast<SaifNet *>(eStk.top());
    auto child = make_unique<SaifPin>(parent->fullName + '.' + key);
    eStk.push(child.get());
    parent->pins[key] = std::move(child);

#ifdef LOG_VERBOSE
    cout << format("pin {}: ", key);
#endif
    while(tokq.front() != ")") saifParse();
#ifdef LOG_VERBOSE
    cout << endl;
#endif
    eStk.pop();
    tokq.pop(buf);
    break;
  }
  case SAIF_T0: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->T0 = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("T0:{} ", ctx->T0);
#endif
    break;
  }
  case SAIF_T1: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->T1 = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("T1:{} ", ctx->T1);
#endif
    break;
  }
  case SAIF_TX: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->TX = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("TX:{} ", ctx->TX);
#endif
    break;
  }
  case SAIF_TZ: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->TZ = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("TZ:{} ", ctx->TZ);
#endif
    break;
  }
  case SAIF_TC: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->TC = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("TC:{} ", ctx->TC);
#endif
    break;
  }
  case SAIF_IG: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->IG = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("IG:{} ", ctx->IG);
#endif
    break;
  }
  case SAIF_TB: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *ctx = dynamic_cast<SaifPin *>(eStk.top());
    ctx->TB = std::stoull(getVal(tokq));
#ifdef LOG_VERBOSE
    cout << format("TB:{} ", ctx->TB);
#endif
    break;
  }
  default:
    SAIF_ERR(true, "Should not reach here");
  }
}

unordered_map<string, SaifInstance *> SaifDB::globalInstanceMap;

SaifInstance *SaifDB::findInstance(const string &name) {
  return globalInstanceMap.contains(name) ? globalInstanceMap[name] : nullptr;
}

const Json::Value &SaifDB::getJson() {
  if(!infoGen) top->getJson(info);
  infoGen = true;
  return info;
}

void SaifPin::getJson(Json::Value &jval) {
  string name;
  if(fullName.find_last_of(".") != string::npos) {
    name = fullName.substr(fullName.find_last_of(".") + 1);
  } else {
    name = fullName;
  }
  jval["type"] = "pin";
  jval["name"] = name;
  jval["T0"] = T0;
  jval["T1"] = T1;
  jval["TX"] = TX;
  jval["TZ"] = TZ;
  jval["TC"] = TC;
  jval["IG"] = IG;
  jval["TB"] = TB;
}

void SaifNet::getJson(Json::Value &jval) {
  for(const auto &[_, p] : pins) {
    Json::Value pjson;
    p->getJson(pjson);
    jval.append(pjson);
  }
}

void SaifInstance::getJson(Json::Value &jval) {
  string name;
  if(fullName.find_last_of(".") != string::npos) {
    name = fullName.substr(fullName.find_last_of(".") + 1);
  } else {
    name = fullName;
  }
  jval["type"] = "instance";
  jval["name"] = name;
  if(nullptr != net) net->getJson(jval["net"]);
  for(const auto &[_, inst] : instances) {
    Json::Value tmp;
    inst->getJson(tmp);
    jval["instances"].append(tmp);
  }
}

}// namespace saif