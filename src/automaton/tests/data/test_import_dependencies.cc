#include <fstream>
#include <iostream>
#include <string>

#include "automaton/core/io/io.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/tests/data/proto_files.h"

#include "gtest/gtest.h"

using automaton::core::data::msg;
using automaton::core::data::schema;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::get_file_contents;

class test_import_dependencies : public ::testing::Test {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    pb_schema1.reset(new protobuf_schema(SCHEMA1_PROTO));
    pb_schema2.reset(new protobuf_schema(SCHEMA2_PROTO));
    pb_schema3.reset(new protobuf_schema(SCHEMA3_PROTO));
    pb_factory.reset(new protobuf_factory());
    setup_schema_dependencies();
  }

  virtual void TearDown() {
    pb_schema1.release();
    pb_schema2.release();
    pb_schema3.release();
    pb_factory.release();
    google::protobuf::ShutdownProtobufLibrary();
  }

  void setup_schema_dependencies() {
    pb_schema2->add_dependency("schema1");
    pb_schema3->add_dependency("schema1");
    pb_schema3->add_dependency("schema2");
    pb_factory->import_schema(pb_schema1.get(), "schema1", "schema1");
    pb_factory->import_schema(pb_schema2.get(), "schema2", "schema2");
    pb_factory->import_schema(pb_schema3.get(), "schema3", "schema3");
  }

  static std::unique_ptr<protobuf_schema> pb_schema1;
  static std::unique_ptr<protobuf_schema> pb_schema2;
  static std::unique_ptr<protobuf_schema> pb_schema3;
  static std::unique_ptr<protobuf_factory> pb_factory;
};

std::unique_ptr<protobuf_schema> test_import_dependencies::pb_schema1;
std::unique_ptr<protobuf_schema> test_import_dependencies::pb_schema2;
std::unique_ptr<protobuf_schema> test_import_dependencies::pb_schema3;
std::unique_ptr<protobuf_factory> test_import_dependencies::pb_factory;

TEST_F(test_import_dependencies, loaded_schema_dependencies) {
}
