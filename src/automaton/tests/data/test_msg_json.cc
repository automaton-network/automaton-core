#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "gtest/gtest.h"

using automaton::core::data::schema;
using automaton::core::data::msg;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;


const char* FIRST_MESSAGE = "first_message";
const char* SECOND_MESSAGE = "second_message";
const char* THIRD_MESSAGE = "third_message";
const char* NESTED_MESSAGE = "nested_message";
const char* FIRST_MESSAGE_NESTED_MESSAGE = "first_message.nested_message";

const char* STRING_FIELD_1 = "string_field_1";
const char* STRING_FIELD_2 = "string_field_2";
const char* STRING_FIELD_NESTED = "string_field_nested";
const char* MESSAGE_FIELD = "message_field";
const char* REPEATED_MSG_FIELD = "repeated_msg_field";
const char* REPEATED_BLOB_FIELD = "repeated_blob_field";

const char* VALUE_1 = "value_1";
const char* VALUE_2 = "value_2";
const char* VALUE_A = "value_A";
const char* VALUE_B = "value_B";
const char* VALUE_NESTED = "value_nested";

class test_msg_json : public ::testing::Test {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    pb_schema.reset(new protobuf_schema());
    pb_factory.reset(new protobuf_factory());
    setup_schema();
  }

  virtual void TearDown() {
    pb_schema.release();
    pb_factory.release();
  }

  /*
    first_message {
      string string_field_1 = 1;
      second_message message_field = 2;
      repeated string repeated_blob_field = 3;

      nested_message {
        string string_field_nested = 1;
      }
    }
    second_message {
      string string_field_2 = 1;
    }
    third_message {
      repeated second_message repeated_msg_field = 1;
    }
  */
  void setup_schema() {
    int m1 = pb_schema->create_message(FIRST_MESSAGE);
    int m2 = pb_schema->create_message(SECOND_MESSAGE);
    int m3 = pb_schema->create_message(NESTED_MESSAGE);
    int m4 = pb_schema->create_message(THIRD_MESSAGE);

    pb_schema->add_scalar_field(
        schema::field_info(1, schema::blob, STRING_FIELD_1, "", false), m1);
    pb_schema->add_scalar_field(
        schema::field_info(1, schema::blob, STRING_FIELD_2, "", false), m2);
    pb_schema->add_scalar_field(
        schema::field_info(1, schema::blob, STRING_FIELD_NESTED, "", false),
        m3);

    pb_schema->add_message_field(schema::field_info(2,
        schema::message_type, MESSAGE_FIELD,
        SECOND_MESSAGE, false), m1);

    pb_schema->add_scalar_field(
        schema::field_info(3, schema::blob, REPEATED_BLOB_FIELD, "", true), m1);

    pb_schema->add_message_field(schema::field_info(1,
        schema::message_type, REPEATED_MSG_FIELD,
        SECOND_MESSAGE, true), m4);

    pb_schema->add_nested_message(m1, m3);
    pb_schema->add_message(m1);
    pb_schema->add_message(m2);
    pb_schema->add_message(m4);
  }

  static std::unique_ptr<protobuf_schema> pb_schema;
  static std::unique_ptr<protobuf_factory> pb_factory;
};

std::unique_ptr<protobuf_schema> test_msg_json::pb_schema;
std::unique_ptr<protobuf_factory> test_msg_json::pb_factory;

TEST_F(test_msg_json, serialize_json) {
  pb_factory->import_schema(pb_schema.get(), "test", "");

  auto msg1 = pb_factory->new_message_by_name(FIRST_MESSAGE);
  auto msg2 = pb_factory->new_message_by_name(SECOND_MESSAGE);
  auto msg3 = pb_factory->new_message_by_name(FIRST_MESSAGE_NESTED_MESSAGE);

  msg1->set_blob(1, VALUE_1);

  msg2->set_blob(1, VALUE_2);
  msg1->set_message(2, *msg2);
  msg1->set_repeated_blob(3, "R1", -1);
  msg1->set_repeated_blob(3, "R2", -1);
  msg1->set_repeated_blob(3, "R3", -1);

  // Deserialize to JSON.
  std::string json;
  msg1->to_json(&json);
  std::cout << json << std::endl;

  // Serialize from JSON.
  auto msg4 = pb_factory->new_message_by_name(FIRST_MESSAGE);
  msg4->from_json(json);
  std::cout << "DESERIALIZED MSG: " << msg4->to_string() << std::endl;

  google::protobuf::ShutdownProtobufLibrary();
}
