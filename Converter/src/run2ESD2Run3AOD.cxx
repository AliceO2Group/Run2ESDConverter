#include "Run3AODConverter.h"
#include <iostream>
#include <memory>
#include <TFile.h>
#include <TTree.h>
#include <cstdio>
#include <cstdlib>

int
main(int argc, char **argv) {
  if (argc != 2) {
    puts("Please specify one or more ROOT file or a list of files preceded by .txt");
    exit(1);
  }

  for (size_t i = 1; i < argc; ++i) {
    auto infile = std::make_unique<TFile>(argv[i]);
    TTree* tEsd = (TTree*)infile->Get("esdTree");
    o2::framework::run2::Run3AODConverter::convert(tEsd, std::cout);
  }
  return 0;
}
