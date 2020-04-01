#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/io/io.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/tests/data/proto_files.h"
#include "gtest/gtest.h"

using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::get_file_contents;

int main(int argc, char* argv[]) {
  try {
    protobuf_factory pb_factory;
    protobuf_schema loaded_schema(TEST_PROTO);
    pb_factory.import_schema(&loaded_schema, "test", "");
    int k = pb_factory.get_schema_id("TestMsg");
    std::cout << "GOT: " << k << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    if (k != 0) {
      return k;
    } else {
      return 0;
    }
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    return 200;
  }
}
