#include <arrow/record_batch.h>
#include <arrow/ipc/reader.h>
#include <arrow/type.h>
#include <arrow/util/io-util.h>

#include <iostream>


int
main(int argc, char **argv) {
  arrow::io::StdinStream inStream;
  std::shared_ptr<arrow::RecordBatchReader> reader;
  auto result = arrow::ipc::RecordBatchStreamReader::Open(&inStream, &reader);
  if (result.ok() == false) {
    std::cout << result.message() << std::endl;
    return 1;
  }
  std::cout << reader->schema()->num_fields() << std::endl;
  return 0;
}
