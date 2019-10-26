#include "Run3AODConverter.h"

#include <arrow/io/buffered.h>
#include <arrow/ipc/writer.h>
#include <arrow/memory_pool.h>
#include <arrow/util/io-util.h>

#include <TEnv.h>
#include <TError.h>
#include <TFile.h>
#include <TTree.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("Please specify one or more ROOT file or a list of files preceded by "
         ".txt");
    exit(1);
  }
  std::vector<std::string> arguments;
  for (auto i = 1u; i < argc; ++i) {
    std::string s{argv[i]};
    arguments.push_back(s);
  }

  size_t nEvents = 0;
  auto pos = std::find(arguments.begin(), arguments.end(), "-n");
  if (pos != arguments.end()) {
    pos++;
    if (pos->empty() == false) {
      nEvents = std::stol(*pos);
      std::cerr << "Events to process: " << nEvents << std::endl;
    } else {
      std::cerr << "Event number not set, using all." << std::endl;
    }
  }

  auto NotAROOTFileName = [](std::string const &input) {
    std::regex isROOTfile(R"(.*\.root$)");
    std::smatch match;
    return !(std::regex_match(input, match, isROOTfile));
  };

  auto pend =
      std::remove_if(arguments.begin(), arguments.end(), NotAROOTFileName);
  arguments.erase(pend, arguments.end());

  gErrorIgnoreLevel = kError;
  gEnv->SetValue("AliRoot.AliLog.Output", "error");

  for (auto &filename : arguments) {
    auto infile = std::make_unique<TFile>(filename.c_str());
    TTree *tEsd = (TTree *)infile->Get("esdTree");
    std::shared_ptr<arrow::io::OutputStream> rawStream(
        new arrow::io::StdoutStream);
    std::shared_ptr<arrow::io::BufferedOutputStream> stream;
    arrow::io::BufferedOutputStream::Create(
        1000000, arrow::default_memory_pool(), rawStream, &stream);
    o2::framework::run2::Run3AODConverter::convert(tEsd, stream, nEvents);
    stream->Close();
  }
  return 0;
}
