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
  try {
    protobuf_schema custom_schema;
    int m1 = custom_schema.create_message("MyMessage");
    custom_schema.add_scalar_field(
        schema::field_info(1, schema::blob, "string_field", "", false), m1);
    custom_schema.add_message_field(
        schema::field_info(
            2, schema::message_type,
            "message_field", "pack1.TestMsg", false),
        m1);
    custom_schema.add_message_field(
        schema::field_info(
            3, schema::message_type,
            "message_field2", "pack2.TestMsg2", false),
        m1);
    custom_schema.add_message(m1);
    custom_schema.add_dependency("name1");
    custom_schema.add_dependency("name2");

    protobuf_schema custom_schema2;
    int m2 = custom_schema2.create_message("TestMsg2");
    custom_schema2.add_scalar_field(schema::field_info(1,
        schema::field_type::blob, "string_field", "", false), m1);
    custom_schema2.add_message(m2);

    protobuf_schema loaded_schema(get_file_contents("automaton/tests/data/test.proto"));
    protobuf_factory pb_factory;
    pb_factory.import_schema(&loaded_schema, "name1", "pack1");
    pb_factory.import_schema(&custom_schema2, "name2", "pack2");
    pb_factory.import_schema(&custom_schema, "name3", "pack3");
    for (uint32_t i = 0; i < pb_factory.get_schemas_number(); i++) {
        std::cout << pb_factory.dump_message_schema(i) << std::endl;
    }
    /*
      int schema_id = pb_factory.get_schema_id("pack3.MyMessage");
      std::string msg = pb_factory.get_message_field_type(schema_id,
          pb_factory.get_field_tag(schema_id, "message_field2"));
      int a = pb_factory.new_message(pb_factory.get_schema_id(msg));
      pb_factory.set_blob(a,1,"alabala");
      std::cout << pb_factory.to_string(a) << std::endl;
      int b = pb_factory.new_message(schema_id);
      pb_factory.set_message(b,3,a);
      std::cout << pb_factory.to_string(b) << std::endl;
  */

    int k; std::cin >> k;
  } catch(std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
