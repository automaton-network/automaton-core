#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/io/io.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "gtest/gtest.h"

using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::get_file_contents;

TEST(protobuf_factory, find_enum) {
  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(get_file_contents("automaton/tests/data/many_enums.proto"));
  pb_factory.import_schema(&loaded_schema, "test", "");
  int id = pb_factory.get_enum_id("enum5");
  std::vector <std::pair<std::string, int> > info = pb_factory.get_enum_values(id);
  EXPECT_EQ(info[0].first, "a");
  EXPECT_EQ(info[0].second, 0);
  EXPECT_EQ(info[1].first, "b");
  EXPECT_EQ(info[1].second, 1);
  google::protobuf::ShutdownProtobufLibrary();
}
