#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/io/io.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"

using automaton::core::data::msg;
using automaton::core::data::schema;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::get_file_contents;

int main(int argc, char* argv[]) {
  protobuf_schema loaded_schema(get_file_contents("automaton/tests/data/many_fields.proto"));
  std::cout << loaded_schema.dump_schema();
  std::cout << "======================" << std::endl;
  protobuf_schema loaded_schema2(get_file_contents("automaton/tests/data/many_enums.proto"));
  std::cout << loaded_schema2.dump_schema();
  google::protobuf::ShutdownProtobufLibrary();
}
