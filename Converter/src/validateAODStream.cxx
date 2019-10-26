#include <arrow/io/buffered.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/writer.h>
#include <arrow/memory_pool.h>
#include <arrow/record_batch.h>
#include <arrow/type.h>
#include <arrow/util/io-util.h>
#include <arrow/util/key_value_metadata.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

using namespace arrow;
using namespace arrow::io;
using namespace arrow::ipc;

int main(int argc, char **argv) {
  std::shared_ptr<InputStream> rawStream(new StdinStream);
  std::shared_ptr<BufferedInputStream> stream;
  auto bufferStatus = BufferedInputStream::Create(
      1000000, default_memory_pool(), rawStream, &stream);
  if (bufferStatus.ok() == false) {
    puts("Unable to create buffer\n");
    return 1;
  }

  arrow::Status status;
  while ((feof(stdin) == false) && (ferror(stdin) == 0)) {
    int64_t pos;
    stream->Tell(&pos);
    printf("Stream position: %lld\n", pos);
    std::shared_ptr<RecordBatchReader> reader;
    status = RecordBatchStreamReader::Open(stream.get(), &reader);
    if (status.ok() == false) {
      std::cerr << "Unable to open stream for read " << status << std::endl;
      char extra[8];
      int64_t readBytes;
      stream->Read(8, &readBytes, extra);
      for (size_t i = 0; i < readBytes; ++i) {
        std::cout << std::hex << extra[i] << " ";
      }
      return 1;
    }
    std::shared_ptr<RecordBatch> batch;

    while (true) {
      printf("Stream position: %lld\n", pos);
      auto readStatus = reader->ReadNext(&batch);
      stream->Tell(&pos);
      if (readStatus.ok() == false) {
        puts("Unable to read batch\n");
        return 1;
      }
      if (batch == nullptr) {
        break;
      };
      std::unordered_map<std::string, std::string> meta;
      batch->schema()->metadata()->ToUnorderedMap(&meta);
      printf("table: %s\n", meta["description"].c_str());
      printf("  num_columns: %d, num_rows: %lld\n", batch->num_columns(),
             batch->num_rows());
    }
    stream->Advance(8 - (pos % 8));
  }

  return 0;
}
