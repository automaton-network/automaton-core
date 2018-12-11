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

/*
  int32 a = 1; -> int32
  uint32 b = 2; -> uint32
  string c = 3; -> blob
  bytes d = 4; -> blob
  fixed32 e = 5; -> uint32
  sint64 f = 6; -> int64
  repeated bool g = 7; -> repeated boolean
*/

const char* TEST_MSG = "TestMsg5";

TEST(protobuf_factory, all_data_types) {
  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(get_file_contents("automaton/tests/data/many_fields.proto"));
  pb_factory.import_schema(&loaded_schema, "test", "");
  int k;
  int id = pb_factory.get_schema_id(TEST_MSG);
  k = pb_factory.get_fields_number(id);
  EXPECT_EQ(k, 7);
  std::string type;
  type = pb_factory.get_field_type(id, 1);
  EXPECT_EQ(type, "int32");
  type = pb_factory.get_field_type(id, 2);
  EXPECT_EQ(type, "uint32");
  type = pb_factory.get_field_type(id, 3);
  EXPECT_EQ(type, "blob");
  type = pb_factory.get_field_type(id, 4);
  EXPECT_EQ(type, "blob");
  type = pb_factory.get_field_type(id, 5);
  EXPECT_EQ(type, "uint32");
  type = pb_factory.get_field_type(id, 6);
  EXPECT_EQ(type, "int64");
  type = pb_factory.get_field_type(id, 7);
  EXPECT_EQ(type, "boolean");

  ///
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_int64(6, 569);
    msg->set_uint32(2, 96);
    msg->set_blob(4, "value");
    msg->set_int32(3, 123);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not int32!");
  }

  // Data field is repeated
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_boolean(7, true);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is repeated!");
  }

  auto msg = pb_factory.new_message_by_name(TEST_MSG);
  msg->set_int32(1, 569);
  msg->set_uint32(2, 123);
  msg->set_blob(3, "alabala");
  msg->set_blob(4, "nica");
  msg->set_uint32(5, 89);
  msg->set_int64(6, -17);
  msg->set_repeated_boolean(7, true);

  try {
    msg->get_uint32(1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not uint32!");
  }
  try {
    msg->get_uint64(2);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not uint64!");
  }
  try {
    msg->get_repeated_blob(3, 0);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not repeated!");
  }
  try {
    msg->get_blob(1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not blob!");
  }
  ////
  EXPECT_EQ(msg->get_int32(1), 569);
  EXPECT_EQ(msg->get_uint32(2), 123);
  EXPECT_EQ(msg->get_blob(3), "alabala");
  EXPECT_EQ(msg->get_blob(4), "nica");
  EXPECT_EQ(msg->get_uint32(5), 89);
  EXPECT_EQ(msg->get_int64(6), -17);
  EXPECT_EQ(msg->get_repeated_boolean(7, 0), true);
  google::protobuf::ShutdownProtobufLibrary();
}
