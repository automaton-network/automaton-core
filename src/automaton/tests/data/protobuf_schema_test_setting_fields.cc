#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/io/io.h"
#include "automaton/core/data/msg.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/tests/data/proto_files.h"

#include "gtest/gtest.h"

using automaton::core::data::msg;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::get_file_contents;

const char* TEST_MSG = "TestMsg";

TEST(protobuf_factory, setting_fields) {
  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(TEST_PROTO);
  pb_factory.import_schema(&loaded_schema, "test", "");

  // *** String functions ***

  // No such field
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_blob(100, "value");
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "No field with tag: 100");
  }

  // Data field is repeated
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_blob(4, "value");
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is repeated!");
  }

  // Get field tag
  {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    EXPECT_EQ(msg->get_field_tag("opt"), 3U);
  }
  // Field is not blob
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_blob(3, "value");
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not blob!");
  }

  /**
    String array functions
  **/

  // No such field
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_blob(100, "value", -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "No field with tag: 100");
  }

  // Data field is not repeated
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_blob(1, "value", -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not repeated!");
  }

  // Field is not blob
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_blob(3, "value", -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not blob!");
  }

  // *** Int32 functions ***

  // No such field
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_int32(100, 42);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "No field with tag: 100");
  }

  // Data field is repeated
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_int32(7, 42);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is repeated!");
  }

  // Field is not int32
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_int32(1, 42);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not int32!");
  }

  // *** Int32 array functions ***

  // No such field
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_int32(100, 42, -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "No field with tag: 100");
  }

  // Data field is not repeated
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_int32(2, 42, -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not repeated!");
  }

  // Field is not int32
  try {
    auto msg = pb_factory.new_message_by_name(TEST_MSG);
    msg->set_repeated_int32(4, 42, -1);
  }
  catch (std::invalid_argument& e) {
    std::string message = e.what();
    std::cerr << message << std::endl;
    EXPECT_EQ(message, "Field is not int32!");
  }
  google::protobuf::ShutdownProtobufLibrary();
}
