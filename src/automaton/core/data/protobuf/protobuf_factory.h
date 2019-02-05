#ifndef AUTOMATON_CORE_DATA_PROTOBUF_PROTOBUF_FACTORY_H_
#define AUTOMATON_CORE_DATA_PROTOBUF_PROTOBUF_FACTORY_H_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/map.h>
#include <google/protobuf/message.h>

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "automaton/core/data/factory.h"
#include "automaton/core/data/protobuf/protobuf_msg.h"
#include "automaton/core/data/schema.h"

namespace automaton {
namespace core {
namespace data {
namespace protobuf {

class protobuf_factory: public factory {
 private:
  google::protobuf::DescriptorPool* pool;
  google::protobuf::DynamicMessageFactory* dynamic_message_factory = nullptr;

  /// Message type name to index in vector<const Message*> schemas
  std::map<std::string, uint32_t> schemas_names;

  /// Message schemas
  std::vector<const google::protobuf::Message*> schemas;

  // Message instances
  // std::vector <protobuf_msg> messages;

  /// Enum type name to index in vector enums
  std::map<std::string, uint32_t> enums_names;

  /// Enums
  std::vector<const google::protobuf::EnumDescriptor*> enums;

  /// Extracts schemas of nested messages
  void extract_nested_messages(const google::protobuf::Descriptor* descriptor);

  /// Extracts schemas of nested enums
  void extract_nested_enums(const google::protobuf::Descriptor* descriptor);

  /**
  Checks if there is protocol buffers data types that we do not support.
  Unsupported types are stored in the enum invalid_types.
  **/
  bool contain_invalid_data(const google::protobuf::Descriptor* descriptor);

  const google::protobuf::Message* get_schema(uint32_t schema_id);

  void import_from_file_proto(google::protobuf::FileDescriptorProto* fileproto,
      const std::string& name, const std::string& package);

 public:
  static const std::map<schema::field_type,
      google::protobuf::FieldDescriptorProto_Type> type_to_protobuf_type;

  static const std::map<google::protobuf::FieldDescriptor::Type,
      schema::field_type> protobuf_type_to_type;

  static const std::map<google::protobuf::FieldDescriptor::CppType,
      schema::field_type> protobuf_ccptype_to_type;

  /**
    TODO(kari): Decide to forbid move & copy constructors.
  **/
  protobuf_factory();
  ~protobuf_factory();

  /*
    This function is used for include schema definitions that were created with
    schema. If the given schema has dependencies, they
    must be imported first or exception will be thrown. Name will be used for
    reference in schema::add_dependency().
  */
  void import_schema(schema* schema, const std::string& name, const std::string& package);

  /**
    following functions are too complicated for mvp.
  **/
  // void import_data(data* data_, const std::string& name);
  // void import_message_to_schema(schema* schema, int id, );
  // void import_enum_to_schema(schema* schema, int id, );

  // std::string serialize_protocol();

  /*
    Prints information about an enum and its values. If the given enum id is not
    valid, exception will be thrown.
  */
  std::string dump_enum(uint32_t enum_id) const;

  /*
    Returns the number of enums in the whole schema definition (both top-level and nested).
  */
  uint32_t get_enums_number() const;

  /*
    Get the id of the enum with enum_name. Id is needed for getting information about the enum
    values. If enum with the given name doesn't exist, exception will be thrown.
  */
  uint32_t get_enum_id(const std::string& enum_name) const;

  /*
    Returns enum value matching value name.

    If no such enum or name exists, exception will be thrown.
  */
  int32_t get_enum_value(uint32_t enum_id, const std::string& value_name) const;

  /*
    Returns a vector of pairs containing info about the values in this enum.
    A pair represents the string name of the value and the int value. If no such
    enum exists, exception will be thrown.
  */
  std::vector<std::pair<std::string, int32_t> > get_enum_values(uint32_t enum_id) const;

  /*
    Prints information about a message schema and its fields. If the given
    schema id is not valid, exception will be thrown.
  */
  std::string dump_message_schema(uint32_t schema_id) const;

  /*
    Returns the number of messages in the whole schema definition (both
    top-level and nested).
  */
  uint32_t get_schemas_number() const;


  /**
    Returns the number of nested message types in the specified message schema.
  */
  uint32_t get_nested_messages_number(uint32_t schema_id) const;

  /**
    Returns the schema id of the corresponding nested message.
  */
  uint32_t get_nested_message_schema_id(uint32_t schema_id, uint32_t index) const;

  /*
    Returns the number of fields in the schema with the given id. If no such
    schema exists, exception will be thrown.
  */
  uint32_t get_fields_number(uint32_t schema_id) const;

  /*
    Returns info about the specified field. Index must be >= 0 and <
    get_fields_number(schema_id). If no such schema or such field exists,
    exception will be thrown.
  */
  schema::field_info get_field_info(uint32_t schema_id, uint32_t index) const;

  /*
    Creates new message from a schema with schema_id. Returns id of the
    created message that is used for setting and getting data,
    serializing and deserializing, etc. If the given schema id is not valid,
    exception will be thrown.
  */
  std::unique_ptr<msg> new_message_by_id(uint32_t schema_id);

  /**
    Creates new message from a schema name.

    Returns the created message that is used for setting and getting data,
    serializing and deserializing, etc.

    If the given schema_name is not valid, exception will be thrown.
  */
  std::unique_ptr<msg> new_message_by_name(const char* schema_name);

  /*
    Creates a copy of the message with the given id. If the given id is not
    valid, exception will be thrown. Returns the id of the created copy.
  */
  uint32_t create_copy_message(int32_t message_id);

  /*
    Returns the id of the schema with schema_name. Id is needed for creating new
    messages of that type, also getting information about the fields. If schema
    with the given name doesn't exist, exception will be thrown.
  */
  uint32_t get_schema_id(const std::string& schema_name) const;

  /*
    Returns the name of the schema of message with the given id. If schema with
    the given name doesn't exist, exception will be thrown.
  */
  std::string get_schema_name(uint32_t schema_id) const;

  /*
    Returns the schema id of the message with the given id. If such message
    doesn't exist, exception will be thrown.
  */
  uint32_t get_message_schema_id(int32_t message_id) const;

  /*
    Returns the type (as a string) of the field with the given tag in the
    schema with the given id. If the given schema id is not valid or there is no
    field with the given tag, exception will be thrown.
    If the type is message, returns 'message'. To get the type of a message
    field use get_message_field_type().
    If the type is enum, returns 'enum'. To get the type of a enum field use
    get_enum_field_type().
    Use the result of this function in set_*() and get_*() and in
    set_repeated_*() and get_repeated_*() if the field is repeated.
  */
  std::string get_field_type(uint32_t schema_id, uint32_t tag) const;

  /*
    If the type of the field is message, returns the fully-qualified name of
    the message type. If the given schema id is not valid or if there is no
    field with the given tag or the field type is not message,
    exception will be thrown.
  */
  std::string get_message_field_type(uint32_t schema_id, uint32_t tag) const;

  /*
    If the type of the field is enum, returns the fully-qualified name of
    the enum type. If the given schema id is not valid or if there is no field
    with the given tag or the field type is not enum, exception will be thrown.
  */
  std::string get_enum_field_type(uint32_t schema_id, uint32_t tag) const;

  /*
    Returns the tag of the field with the given name in the schema with the
    given id. If the given schema id is not valid or there is no field with the
    given name, exception will be thrown.
  */
  uint32_t get_field_tag(uint32_t schema_id, const std::string& name) const;

  /*
    Returns true if the field is repeated, false, otherwise. If the given schema
    id is not valid or if there is no field with the given tag, exception will
    be thrown.
  */
  bool is_repeated(uint32_t schema_id, uint32_t tag) const;
};

}  // namespace protobuf
}  // namespace data
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_DATA_PROTOBUF_PROTOBUF_FACTORY_H_
