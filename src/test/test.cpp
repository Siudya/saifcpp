#include "fmt/core.h"
#include "saif.h"
#include <chrono>
#include <iostream>
#include <string>

using fmt::format;
using namespace std::chrono;
int main(int argc, const char *argv[]) {
  if(argc < 2) {
    std::cout << "Please input SAIF file" << std::endl;
    return 1;
  }
  std::string file = argv[1];
  std::cout << format("Parsing {}", file) << std::endl;
  auto st = system_clock::now();
  saif::SaifDB db;
  db.parse(file);
  auto et = system_clock::now();
  auto elapsedM = duration_cast<minutes>(et - st).count();
  auto elapsedS = duration_cast<seconds>(et - st).count();
  auto elapsedMs = duration_cast<milliseconds>(et - st).count();

  if(elapsedM > 10) {
    std::cout << format("[INFO] Time elapsed: {}m", elapsedM) << std::endl;
  } else if(elapsedS > 10) {
    std::cout << format("[INFO] Time elapsed: {}s", elapsedS) << std::endl;
  } else {
    std::cout << format("[INFO] Time elapsed: {}ms", elapsedMs) << std::endl;
  }
}