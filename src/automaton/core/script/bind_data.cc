#include "automaton/core/script/engine.h"

#include "automaton/core/data/factory.h"
#include "automaton/core/data/msg.h"
#include "automaton/core/io/io.h"

using automaton::core::data::msg;
using automaton::core::data::factory;
using automaton::core::data::schema;

using std::shared_ptr;
using std::unique_ptr;

namespace automaton {
namespace core {
namespace script {

void engine::bind_data() {
  auto msg_type = create_simple_usertype<msg>();

  msg_type.set(sol::meta_function::index,
    [this](sol::this_state L, msg& m, std::string key) -> sol::object {
      VLOG(9) << "Getting key: " << key;
      sol::state_view lua(L);
      auto tag_id = m.get_field_tag(key);
      auto fi = m.get_field_info_by_tag(tag_id);
      auto ftype = fi.type;
      switch (ftype) {
        case schema::int32: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_int32(tag_id, i));
            }
            return sol::make_object(L, result);
          } else {
            return sol::make_object(L, m.get_int32(tag_id));
          }
        }
        case schema::int64: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_int64(tag_id, i));
            }
            return sol::make_object(L, result);
          } else {
            return sol::make_object(L, m.get_int64(tag_id));
          }
        }
        case schema::uint32: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_uint32(tag_id, i));
            }
            return sol::make_object(L, result);
          } else {
            return sol::make_object(L, m.get_uint32(tag_id));
          }
        }
        case schema::uint64: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_uint64(tag_id, i));
            }
            return result;
          } else {
            return sol::make_object(L, m.get_uint64(tag_id));
          }
        }
        case schema::blob: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_blob(tag_id, i));
            }
            return sol::make_object(L, result);
          } else {
            return sol::make_object(L, m.get_blob(tag_id));
          }
        }
        case schema::message_type: {
          if (fi.is_repeated) {
            auto n = m.get_repeated_field_size(tag_id);
            sol::table result = create_table();
            for (uint32_t i = 0; i < n; i++) {
              result.add(m.get_repeated_message(tag_id, i));
            }
            return sol::make_object(L, result);
          } else {
            return sol::make_object(L, m.get_message(tag_id));
          }
        }
        default: {
          return sol::make_object(L, sol::lua_nil);
        }
      }
      return sol::make_object(L, sol::lua_nil);
    });

  msg_type.set(sol::meta_function::new_index,
    [this](sol::this_state L, msg& m, std::string key, sol::object value) {
      VLOG(9) << "Setting key:" << key << " value: " << value.as<std::string>();
      auto tag_id = m.get_field_tag(key);
      auto fi = m.get_field_info_by_tag(tag_id);
      auto ftype = fi.type;
      switch (ftype) {
        case schema::int32: {
          int n = value.as<int>();
          if (fi.is_repeated) {
            m.set_repeated_int32(tag_id, n, -1);
          } else {
            m.set_int32(tag_id, n);
          }
          break;
        }
        case schema::int64: {
          auto n = value.as<int64_t>();
          if (fi.is_repeated) {
            m.set_repeated_int64(tag_id, n, -1);
          } else {
            m.set_int64(tag_id, n);
          }
          break;
        }
        case schema::uint32: {
          auto n = value.as<uint32_t>();
          if (fi.is_repeated) {
            m.set_repeated_uint32(tag_id, n, -1);
          } else {
            m.set_uint32(tag_id, n);
          }
          break;
        }
        case schema::uint64: {
          auto n = value.as<uint64_t>();
          if (fi.is_repeated) {
            m.set_repeated_uint64(tag_id, n, -1);
          } else {
            m.set_uint64(tag_id, n);
          }
          break;
        }
        case schema::blob: {
          // TOD(asen): Check whether string_view is faster.
          auto blob = value.as<std::string>();
          if (fi.is_repeated) {
            m.set_repeated_blob(tag_id, blob, -1);
          } else {
            m.set_blob(tag_id, blob);
          }
          break;
        }
        case schema::message_type: {
          auto message = value.as<msg*>();
          if (fi.is_repeated) {
            m.set_repeated_message(tag_id, *message, -1);
          } else {
            m.set_message(tag_id, *message);
          }
          break;
        }
        default: {
          LOG(ERROR) << "WAT?";
        }
      }
    });

  msg_type.set("set_blob", &msg::set_blob);
  msg_type.set("get_blob", &msg::get_blob);
  msg_type.set("set_repeated_blob", &msg::set_repeated_blob);
  msg_type.set("get_repeated_blob", &msg::get_repeated_blob);

  msg_type.set("get_int32", &msg::get_int32);
  msg_type.set("set_int32", &msg::set_int32);
  msg_type.set("get_repeated_int64", &msg::get_repeated_int32);
  msg_type.set("set_repeated_int64", &msg::set_repeated_int32);

  msg_type.set("get_uint32", &msg::get_uint32);
  msg_type.set("set_uint32", &msg::set_uint32);
  msg_type.set("get_repeated_uint64", &msg::get_repeated_uint32);
  msg_type.set("set_repeated_uint64", &msg::set_repeated_uint32);

  msg_type.set("get_int64", &msg::get_int64);
  msg_type.set("set_int64", &msg::set_int64);
  msg_type.set("get_repeated_int64", &msg::get_repeated_int64);
  msg_type.set("set_repeated_int64", &msg::set_repeated_int64);

  msg_type.set("get_uint64", &msg::get_uint64);
  msg_type.set("set_uint64", &msg::set_uint64);
  msg_type.set("get_repeated_uint64", &msg::get_repeated_uint64);
  msg_type.set("set_repeated_uint64", &msg::set_repeated_uint64);

  msg_type.set("get_bool", &msg::get_boolean);
  msg_type.set("set_bool", &msg::set_boolean);
  msg_type.set("get_repeated_bool", &msg::get_repeated_boolean);
  msg_type.set("set_repeated_bool", &msg::set_repeated_boolean);

  msg_type.set("get_enum", &msg::get_enum);
  msg_type.set("set_enum", &msg::set_enum);
  msg_type.set("get_repeated_enum", &msg::get_repeated_enum);
  msg_type.set("set_repeated_enum", &msg::set_repeated_enum);

  msg_type.set("get_msg", &msg::get_message);
  msg_type.set("set_msg", &msg::set_message);
  msg_type.set("get_repeated_msg", &msg::get_repeated_message);
  msg_type.set("set_repeated_msg", &msg::set_repeated_message);

  msg_type.set("serialize", [](msg& m) {
      std::string s;
      m.serialize_message(&s);
      return s;
    });

  msg_type.set("deserialize", &msg::deserialize_message);

  msg_type.set("to_json", [](msg& m) {
      std::string json;
      m.to_json(&json);
      return json;
    });

  msg_type.set("to_string", &msg::to_string);

  set_usertype("msg", msg_type);
}

}  // namespace script
}  // namespace core
}  // namespace automaton
