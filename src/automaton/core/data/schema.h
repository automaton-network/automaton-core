#ifndef AUTOMATON_CORE_DATA_SCHEMA_H__
#define AUTOMATON_CORE_DATA_SCHEMA_H__

#include <map>
#include <string>
#include <vector>

namespace automaton {
namespace core {
namespace data {

/*
  TODO(kari): how to make difference between schema as a whole combo
  of message and enum schemas, and a single message/enum schema definition.
  It may be confusing.
  TODO(kari): field_info may become class. There are too many functions
  related to fields:
    * get_field_tag()
    * get_field_type_by_tag/name()
    * is_repeated()
    and others
  TODO(kari): After making dump functions returning string, see if there are to_string functions
  that do the same.

*/

class schema {
 public:
  virtual ~schema() = 0;

  /**
   Allowed data types.
  **/
  enum field_type {
    unknown = 0,
    message_type = 1,
    enum_type = 2,
    blob = 4,
    int32 = 5,
    int64 = 6,
    uint32 = 7,
    uint64 = 8,
    boolean = 9,
  };

  /**
   Struct representing one field in a message schema (field schema). Does NOT
   contain value, only describes a field.
   Tag - unique number within a message schema. Identifies the field.
   Type - one of the allowed data types. If it is a message or enum type,
     its message or enum type should be stored in fully_qualified_type.
   Name - unique name within a message schema. Identifies the field.
   Fully_qualified_type - specifies the type of message and enum fields. It is
     ignored for scalar types.
  */
  struct field_info {
    uint32_t tag;
    field_type type;
    std::string name;
    std::string fully_qualified_type;
    bool is_repeated;
    field_info(uint32_t tag, field_type type, const std::string& name,
        const std::string& fully_qualified_type, bool is_repeated);
    std::string type_name() {
      switch (type) {
        case unknown: return "unknown";
        case message_type: return "message";
        case enum_type: return "enum";
        case blob: return "blob";
        case int32: return "int32";
        case int64: return "int64";
        case uint32: return "uint32";
        case uint64: return "uint64";
        case boolean: return "bool";
        default: return "N/A";
      }
    }
  };

  /**
    Serializes schema to JSON string.
  */
  virtual bool to_json(std::string* output) const = 0;

  /**
    Deserializes schema from JSON string.
  */
  virtual bool from_json(const std::string& input) = 0;

  virtual std::string dump_schema() = 0;

  /**
    If this schema (schema A) depends on another one (schema B), it must be
    added here. Schema_name is the name that has been/will be given to the
    schema (B) on import (when schema::import_schema_definition() is called).
    Shema B should be imported before Schema A.
  **/
  virtual void add_dependency(const std::string& schema_name) = 0;

  /**
    Returns the id of the created message_schema.
    TODO(kari): Check for name collisions.
    Only creates empty message schema and name it. Fields and/or nested
    enums/messages should be added to the message (using add_* and
    add_*_field) and then add_message() so the created message schema is added
    to the schema.
  **/
  virtual uint32_t create_message(const std::string& message_name) = 0;

  /**
    Returns the id of the created enum_schema.
    TODO(kari): Check for name collisions.
    Only creates empty enum schema and name it. Values should be added to the
    enum (using add_enum_value) and then add_enum() so the created enum schema
    is added to the schema.
  **/
  virtual uint32_t create_enum(const std::string& enum_name) = 0;

  /**
    Used to add values to an already created enum with enum_id. If such enum
    doesn't exist, exception will be thrown.
    TODO(kari): Decide if duplicate values are allowed.
  **/
  virtual void add_enum_value(uint32_t enum_id, const std::string& value_name, int32_t value) = 0;

  /**
    Used to add nested message. Both messages must already exist. If any of
    them doesn't exist, exception will be thrown. The message with id
    sub_message_id will be added as a nested message to the other.
    *** Nested message is different from message field. It is like an inner
    class, NOT like a reference to an object ***
    TODO(kari): Check for name collisions
    TODO(kari): Decide if duplicate values are allowed.
  **/
  virtual void add_nested_message(int32_t message_id, uint32_t sub_message_id) = 0;

  /**
    Used to add an already created message/enum schema to this schema. The
    message/enum should first be created and ready (fields/ nested messages/
    enums or enum values must be added) before calling this function.
    Enum can be added to another message and if no message_id is provided in
    add_enum or message_id = -1, enum will be added globally. If message/enum
    with the given id doesn't exist, exception will be thrown.
  **/
  virtual void add_message(int32_t message_id) = 0;
  virtual void add_enum(uint32_t enum_id, int32_t message_id = -1) = 0;

  /**
    These functions are called to add fields to a message. Any of them can be
    called multiple times on the same message. If the given message_id is not
    valid or the given field doesn't match the expected field type (ex.
    add_message_field() is called but the provided field is scalar type),
    exception will be thrown.
  **/
  virtual void add_scalar_field(field_info field, int32_t message_id) = 0;
  virtual void add_enum_field(field_info field, int32_t message_id) = 0;
  virtual void add_message_field(field_info field, int32_t message_id) = 0;

  virtual std::vector<std::string> get_message_names() = 0;
};

}  // namespace data
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_DATA_SCHEMA_H__
