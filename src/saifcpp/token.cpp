#include "boost/tokenizer.hpp"
#include "fmt/core.h"
#include "saif.h"
#include <iostream>
#include <thread>

using namespace std;
namespace saif {

const boost::char_separator<char> saifSep(" \r\n\t\"", "()");

void TokenQueue::start(const string &file) {
  ifs.open(file, ios::in);
  SAIF_ERR(!ifs.is_open(), "Can not open {}", file);
  auto fn = [](TokenQueue *q) -> void { q->load(); };
  auto t = thread(fn, this);
  t.detach();
}

void TokenQueue::load() {
  string buf = "";
  while(getline(ifs, buf)) {
    boost::tokenizer tok(buf, saifSep);
    for(const auto &t : tok) {
      while(!q.push(t));
    }
  }
  fileEnd = true;
}

void TokenQueue::pop(std::string &buf) {
  if(headCacheValid) {
    buf = headCache;
    headCacheValid = false;
  } else {
    uint32_t to = 0;
    while(!q.pop(buf)) {
      usleep(10);
      to++;
      SAIF_ERR(to > 1000, "Token queue read time out!");
    }
  }
}

string TokenQueue::front() {
  if(!headCacheValid) {
    pop(headCache);
    headCacheValid = true;
  }
  return headCache;
}

bool TokenQueue::empty() {
  return fileEnd && q.empty() && !headCacheValid;
}
}// namespace saif