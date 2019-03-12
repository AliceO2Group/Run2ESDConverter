#include "Run3AODConverter.h"
#include <arrow/io/buffered.h>
#include <arrow/util/io-util.h>
#include <arrow/ipc/writer.h>

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
    std::shared_ptr<arrow::io::OutputStream> rawStream(new arrow::io::StdoutStream);
    std::shared_ptr<arrow::io::BufferedOutputStream> stream;
    arrow::io::BufferedOutputStream::Create(1000000, arrow::default_memory_pool(), rawStream, &stream);
    o2::framework::run2::Run3AODConverter::convert(tEsd, stream);
    stream->Close();
  }
  return 0;
}
