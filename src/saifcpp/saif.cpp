#include "saif.h"
#include "fmt/core.h"
#include <boost/token_functions.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using fmt::format;

namespace saif {

const std::unordered_map<std::string, state_t> lexMap{
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

void SaifDB::parse(const string &file) {
  tokq.start(file);
  saifParse();
}

void nameNormalize(std::string &str) {
  std::size_t found = str.find("\\[");
  while(found != std::string::npos) {
    str.erase(found, 1);
    found = str.find("\\[");
  }
  found = str.find("\\]");
  while(found != std::string::npos) {
    str.erase(found, 1);
    found = str.find("\\]");
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

void SaifDB::saifParse() {
  string buf = "";
  string key = "";
  tokq.pop(buf);
  tokq.pop(key);
  SAIF_ERR(buf != "(", "Illegal start of domain {}", key);
  state_t state = lexMap.contains(key) ? lexMap.at(key) : SAIF_PIN;
  switch(state) {
  case SAIF_FILE: {
    saifParse();
    tokq.pop(buf);
    SAIF_ERR(buf != ")", "Illegal end of file");
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
    duration = std::stoull(getVal(tokq));
    saifParse();
    break;
  }
  case SAIF_INSTANCE: {
    tokq.pop(buf);
    nameNormalize(buf);
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
      parent->instances[buf] = std::move(child);
    }
#ifndef NDEBUG
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
    nameNormalize(key);
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_NET;
    SAIF_ERR(failed, "Illegal PIN declaration!");
    SaifNet *parent = dynamic_cast<SaifNet *>(eStk.top());
    SAIF_ERR(parent->pins.contains(key), "PIN {} has already been parsed", key);
    auto child = make_unique<SaifPin>(parent->fullName + '.' + key);
    eStk.push(child.get());
    parent->pins[key] = std::move(child);
#ifndef NDEBUG
    cout << format("pin {}: ", key);
#endif
    while(tokq.front() != ")") saifParse();
#ifndef NDEBUG
    cout << endl;
#endif
    eStk.pop();
    tokq.pop(buf);
    break;
  }
  case SAIF_T0: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->T0 = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("T0:{} ", parent->T0);
#endif
    break;
  }
  case SAIF_T1: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->T1 = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("T1:{} ", parent->T1);
#endif
    break;
  }
  case SAIF_TX: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->TX = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("TX:{} ", parent->TX);
#endif
    break;
  }
  case SAIF_TZ: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->TZ = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("TZ:{} ", parent->TZ);
#endif
    break;
  }
  case SAIF_TC: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->TC = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("TC:{} ", parent->TC);
#endif
    break;
  }
  case SAIF_IG: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->IG = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("IG:{} ", parent->IG);
#endif
    break;
  }
  case SAIF_TB: {
    bool failed = eStk.empty() || eStk.top()->domain() != SAIF_PIN;
    SAIF_ERR(failed, "Illegal ACT declaration!");
    SaifPin *parent = dynamic_cast<SaifPin *>(eStk.top());
    parent->T0 = std::stoull(getVal(tokq));
#ifndef NDEBUG
    cout << format("TB:{} ", parent->TB);
#endif
    break;
  }
  default:
    SAIF_ERR(true, "Should not reach here");
  }
}

}// namespace saif