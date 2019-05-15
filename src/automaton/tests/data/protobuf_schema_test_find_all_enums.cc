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

TEST(protobuf_factory, find_all_enums) {
  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(MANY_ENUMS_PROTO);
  pb_factory.import_schema(&loaded_schema, "test", "");
  int k = pb_factory.get_enums_number();
  EXPECT_EQ(k, 8);
  google::protobuf::ShutdownProtobufLibrary();
}
