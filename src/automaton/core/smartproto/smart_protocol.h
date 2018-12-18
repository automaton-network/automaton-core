#ifndef AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_
#define AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/data/protobuf/protobuf_schema.h"

namespace automaton {
namespace core {
namespace smartproto {

class smart_protocol {
 public:
  struct cmd {
    std::string name;
    std::string input_type;
    std::string output_type;

    cmd(std::string nm, std::string input, std::string output):name(nm), input_type(input), output_type(output) {}
  };

  static std::shared_ptr<smart_protocol> get_protocol(std::string proto_id);
  static std::vector<std::string> list_protocols();

  static bool load(std::string path);

  std::unordered_map<std::string, std::string> get_msgs_definitions();
  std::vector<data::schema*> get_schemas();
  std::vector<std::string> get_scripts();
  std::vector<std::string> get_wire_msgs();
  std::vector<std::string> get_commands();

 private:
  smart_protocol();
  static std::unordered_map<std::string, std::shared_ptr<smart_protocol>> protocols;
  std::string id;
  uint32_t update_time_slice;

  std::vector<std::string> lua_scripts;
  std::unordered_map<std::string, std::string> msgs_defs;
  std::vector<data::schema*> schemas;
  std::vector<std::string> wire_msgs;
  std::vector<cmd> commands;
};

}  // namespace smartproto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_
