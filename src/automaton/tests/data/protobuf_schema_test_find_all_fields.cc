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

TEST(protobuf_factory, find_all_fields) {
  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(get_file_contents("automaton/tests/data/many_fields.proto"));
  pb_factory.import_schema(&loaded_schema, "test", "");
  int k;
  int id = pb_factory.get_schema_id("TestMsg");
  k = pb_factory.get_fields_number(id);
  EXPECT_EQ(k, 0);
  id = pb_factory.get_schema_id("TestMsg2");
  k = pb_factory.get_fields_number(id);
  EXPECT_EQ(k, 7);
  id  = pb_factory.get_schema_id("TestMsg4.TestMsg5");
  k = pb_factory.get_fields_number(id);
  EXPECT_EQ(k, 1);
  id  = pb_factory.get_schema_id("TestMsg4.TestMsg5.TestMsg6");
  k = pb_factory.get_fields_number(id);
  EXPECT_EQ(k, 1);
  google::protobuf::ShutdownProtobufLibrary();
}
