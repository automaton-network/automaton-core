#ifndef AUTOMATON_CORE_DATA_MSG_H__
#define AUTOMATON_CORE_DATA_MSG_H__

#include <memory>
#include <string>

#include "automaton/core/data/schema.h"

namespace automaton {
namespace core {
namespace data {

/**
  Schema message interface
*/
class msg {
 public:
  msg() {}

  // TODO(asen): Produce a copy of the message.
  // Disable copy constructor for now.
  msg(const msg& that) = delete;

  virtual ~msg() = 0;

  virtual uint32_t get_schema_id() const = 0;

  /**
    Returns the name of the message schema.

    If schema with the given name doesn't exist, exception will be thrown.
  */
  virtual std::string get_message_type() const = 0;

  /** Returns the size of the repeated field in the given message. If no such
      message or field exists or if the field is not repeated, exception will be
      thrown.
  */
  virtual uint32_t get_repeated_field_size(uint32_t field_tag) const = 0;

  /** Serializes the given message into the given string. If no such message
      exists or some error while serializing happens, exception will be thrown.
  */
  virtual bool serialize_message(std::string* output) const = 0;

  /** Deserializes message from the given string into a message with the given
      message id. If no such message exists or some error while deserializing
      happens, exception will be thrown. To use this function you should first
      create new message from the same schema and pass the id to this function.
  */
  virtual bool deserialize_message(const std::string& input) = 0;

  /**
    Serializes message to JSON string.
  */
  virtual bool to_json(std::string* output) const = 0;

  /**
    Deserializes message from JSON string.
  */
  virtual bool from_json(const std::string& input) = 0;

  /** Returns human readable representation of the message with the given id. If
      message id is not valid, exception will be thrown.
  */
  virtual std::string to_string() const = 0;

  /**
    Setters and getters for the scalar types. If any tag or id is invalid, or if
    you try to use these functions on non-matching type fields, or if you try to
    use get/set_repeated_*() on non-repeated field or vise versa, exception will
    be thrown. If you use out of range index on set_repeated_*() the value will
    just be added at the end (not on the specified index). If you use out of
    range index on set_repeated_*(), exception will be thrown.
  */

  /// Blob (string, bytes, ...)

  virtual void set_blob(uint32_t field_tag, const std::string& value) = 0;

  virtual std::string get_blob(uint32_t field_tag) const = 0;

  virtual void set_repeated_blob(uint32_t field_tag, const std::string& value, int32_t index = -1)
      = 0;

  virtual std::string get_repeated_blob(uint32_t field_tag, int32_t index) const = 0;

  /// Int 32

  virtual void set_int32(uint32_t field_tag, int32_t value) = 0;

  virtual int32_t get_int32(uint32_t field_tag) const = 0;

  virtual void set_repeated_int32(uint32_t field_tag, int32_t value, int32_t index = -1) = 0;

  virtual int32_t get_repeated_int32(uint32_t field_tag, int32_t index) const = 0;

  /// uint32_t 32

  virtual void set_uint32(uint32_t field_tag, uint32_t value) = 0;

  virtual uint32_t get_uint32(uint32_t field_tag) const = 0;

  virtual void set_repeated_uint32(uint32_t field_tag, uint32_t value, int32_t index = -1) = 0;

  virtual uint32_t get_repeated_uint32(uint32_t field_tag, int32_t index) const = 0;

  /// Int 64

  virtual void set_int64(uint32_t field_tag, int64_t value) = 0;

  virtual int64_t get_int64(uint32_t field_tag) const = 0;

  virtual void set_repeated_int64(uint32_t field_tag, int64_t value, int32_t index = -1) = 0;

  virtual int64_t get_repeated_int64(uint32_t field_tag, int32_t index) const = 0;

  /// uint32_t 64

  virtual void set_uint64(uint32_t field_tag, uint64_t value) = 0;

  virtual uint64_t get_uint64(uint32_t field_tag) const = 0;

  virtual void set_repeated_uint64(uint32_t field_tag, uint64_t value, int32_t index = -1) = 0;

  virtual uint64_t get_repeated_uint64(uint32_t field_tag, int32_t index) const = 0;

  /// Boolean

  virtual void set_boolean(uint32_t field_tag, bool value) = 0;

  virtual bool get_boolean(uint32_t field_tag) const = 0;

  virtual void set_repeated_boolean(uint32_t field_tag, bool value, int32_t index = -1) = 0;

  virtual bool get_repeated_boolean(uint32_t field_tag, int32_t index) const = 0;

  /*
    Setters and getters for message type fields. When you use setters, you
    should already have the sub_message created and initialized. This function
    makes a copy of it so if you change the sub_message, the original message
    that keeps the copy will not be affected. If any tag or id is invalid, or if
    you try to use these functions on non-message fields, or if you try to use
    get/set_repeated_message() on non-repeated field or vise versa, or if you
    give sub_message that is from different type than expected, exception will
    be thrown. If you use out of range index on set_repeated_message() the value
    will just be added at the end (not on the specified index). If you use out
    of range index on get_repeated_message(), exception will be thrown.
  */
  virtual void set_message(uint32_t field_tag, const msg& sub_message) = 0;

  virtual std::unique_ptr<msg> get_message(uint32_t field_tag) const = 0;

  virtual void set_repeated_message(uint32_t field_tag, const msg& sub_message, int32_t index = -1)
      = 0;

  virtual std::unique_ptr<msg> get_repeated_message(uint32_t field_tag, int32_t index) const = 0;

  /*
    Setters and getters for enum type fields. If any tag or id is invalid, or if
    you try to use these functions on non-enum fields, exception will be thrown.
    If you use out of range index on set_repeated_enum() the value will just be
    added at the end (not on the specified index). If you use out of range index
    on get_repeated_enum(), exception will be thrown.
  */
  virtual void set_enum(uint32_t field_tag, int32_t value) = 0;

  virtual int32_t get_enum(uint32_t field_tag) const = 0;

  virtual void set_repeated_enum(uint32_t field_tag, int32_t value, int32_t index = -1) = 0;

  virtual int32_t get_repeated_enum(uint32_t field_tag, int32_t index) const = 0;

  virtual uint32_t get_field_tag(const std::string& name) const = 0;

  virtual schema::field_info get_field_info_by_tag(uint32_t field_tag) const = 0;
};

}  // namespace data
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_DATA_MSG_H__
