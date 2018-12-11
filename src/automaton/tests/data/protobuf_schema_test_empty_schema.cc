#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "gtest/gtest.h"

using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;

int main(int argc, char* argv[]) {
  try {
    protobuf_schema custom_schema;
    protobuf_factory pb_factory;
    pb_factory.import_schema(&custom_schema, "test", "");
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    return 1;
  }
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
