#include "argparse/argparse.hpp"
#include "fmt/core.h"
#include "saif.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using fmt::format;
using namespace std::chrono;
using namespace std;

template <typename T>
void timeElapsed(time_point<T> &st) {
  auto et = system_clock::now();
  auto elapsedM = duration_cast<minutes>(et - st).count();
  auto elapsedS = duration_cast<seconds>(et - st).count();
  auto elapsedMs = duration_cast<milliseconds>(et - st).count();
  if(elapsedM > 10) {
    cout << format("[INFO] Time elapsed: {}m", elapsedM) << endl;
  } else if(elapsedS > 10) {
    cout << format("[INFO] Time elapsed: {}s", elapsedS) << endl;
  } else {
    cout << format("[INFO] Time elapsed: {}ms", elapsedMs) << endl;
  }
}

int main(int argc, const char *argv[]) {
  auto argparser = argparse::ArgumentParser("Saif parser", "1.0.0", argparse::default_arguments::help);
  argparser.add_argument("saif").help("Design SAIF file");
  argparser.add_argument("-o", "--out").default_value(string("")).help("Output json file");
  try {
    argparser.parse_args(argc, argv);
  } catch(const exception &err) {
    cerr << err.what() << endl;
    cerr << argparser;
    exit(1);
  }
  auto sfile = argparser.get<string>("saif");
  auto jfile = argparser.get<string>("--out");

  auto st = system_clock::now();
  saif::SaifDB db;
  db.parse(sfile);
  cout << "Parsing " << sfile << endl;
  timeElapsed(st);

  if(!jfile.empty()) {
    if(jfile == "-") {
      cout << db.getJson().toStyledString() << endl;
    } else {
      ofstream ofs;
      ofs.open(jfile, ios::out);
      if(ofs.is_open()) {
        cout << "Exporting JSON file " << jfile << endl;
        ofs << db.getJson().toStyledString() << endl;
        ofs.close();
      } else {
        cerr << jfile << " cannot be opened" << endl;
        exit(1);
      }
    }
    timeElapsed(st);
  }
}