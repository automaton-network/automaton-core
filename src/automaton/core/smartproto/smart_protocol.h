#ifndef AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_
#define AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/data/factory.h"
#include "automaton/core/data/schema.h"

namespace automaton {
namespace core {
namespace smartproto {

class smart_protocol {
 public:
  struct cmd {
    std::string name;
    std::string input_type;
    std::string output_type;

    cmd(const std::string& nm, const std::string& input, const std::string& output):name(nm), input_type(input), output_type(output) {}
  };

  static std::shared_ptr<smart_protocol> get_protocol(const std::string& proto_id);
  static std::vector<std::string> list_protocols();

  static bool load(const std::string& id, const std::string& path);

  std::shared_ptr<data::factory> get_factory();
  std::unordered_map<std::string, std::string> get_msgs_definitions();
  std::vector<data::schema*> get_schemas();
  std::vector<std::string> get_scripts();
  std::vector<std::string> get_wire_msgs();
  std::vector<std::string> get_commands();
  std::string get_configuration_file();
  uint32_t get_update_time_slice();

 private:
  smart_protocol();
  static std::unordered_map<std::string, std::shared_ptr<smart_protocol>> protocols;
  std::string id;
  std::shared_ptr<data::factory> factory;
  uint32_t update_time_slice;

  std::vector<std::string> lua_scripts;
  std::unordered_map<std::string, std::string> msgs_defs;
  std::vector<data::schema*> schemas;
  std::vector<std::string> wire_msgs;
  std::vector<cmd> commands;
  std::string config_json;
};

}  // namespace smartproto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_SMARTPROTO_SMART_PROTOCOL_H_
