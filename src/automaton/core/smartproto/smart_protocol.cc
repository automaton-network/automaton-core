#include "automaton/core/smartproto/smart_protocol.h"

#include <fstream>
#include <string>

#include <json.hpp>

#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/io/io.h"

using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::schema;
using automaton::core::io::get_file_contents;

namespace automaton {
namespace core {
namespace smartproto {

std::unordered_map<std::string, std::shared_ptr<smart_protocol> > smart_protocol::protocols;

smart_protocol::smart_protocol() {
  factory = std::shared_ptr<data::factory>(new protobuf_factory());
}

std::shared_ptr<smart_protocol> smart_protocol::get_protocol(std::string proto_id) {
  auto it = protocols.find(proto_id);
  if (it == protocols.end()) {
    return nullptr;
  }
  return it->second;
}
std::vector<std::string> smart_protocol::list_protocols() {
  std::vector<std::string> result;
  for (auto p : protocols) {
    result.push_back(p.first);
  }
  return result;
}

bool smart_protocol::load(std::string path) {
  std::shared_ptr<smart_protocol> proto(new smart_protocol());
  std::ifstream i(path + "config.json");
  if (!i.is_open()) {
    LOG(ERROR) << "Error while opening " << path << "config.json";
    return false;
  } else {
    proto->config_json = std::string(std::istreambuf_iterator<char>(i), {});
    i.seekg(0);
    nlohmann::json j;
    i >> j;
    i.close();
    proto->update_time_slice = j["update_time_slice"];
    std::vector<std::string> schemas_filenames = j["schemas"];
    std::vector<std::string> lua_scripts_filenames = j["lua_scripts"];
    std::vector<std::string> wm = j["wire_msgs"];
    proto->wire_msgs = wm;
    for (auto cmd : j["commands"]) {
      proto->commands.push_back({cmd[0], cmd[1], cmd[2]});
    }

    for (uint32_t i = 0; i < schemas_filenames.size(); ++i) {
      std::string file_content = get_file_contents((path + schemas_filenames[i]).c_str());
      proto->msgs_defs[schemas_filenames[i]] = file_content;
      proto->schemas.push_back(new protobuf_schema(file_content));
      proto->factory->import_schema(proto->schemas.back(), "", "");
    }

    for (uint32_t i = 0; i < lua_scripts_filenames.size(); ++i) {
      proto->lua_scripts.push_back(get_file_contents((path + lua_scripts_filenames[i]).c_str()));
    }
  }
  protocols[path] = proto;
  return true;
}

std::shared_ptr<data::factory> smart_protocol::get_factory() {
  return factory;
}

std::unordered_map<std::string, std::string> smart_protocol::get_msgs_definitions() {
  return msgs_defs;
}

std::vector<data::schema*> smart_protocol::get_schemas() {
  return schemas;
}
std::vector<std::string> smart_protocol::get_scripts() {
  return lua_scripts;
}
std::vector<std::string> smart_protocol::get_wire_msgs() {
  return wire_msgs;
}
std::vector<std::string> smart_protocol::get_commands() {
  std::vector<std::string> cmds;
  for (auto c : commands) {
    cmds.push_back(c.name);
  }
  return cmds;
}

std::string smart_protocol::get_configuration_file() {
  return config_json;
}

uint32_t smart_protocol::get_update_time_slice() {
  return update_time_slice;
}

}  // namespace smartproto
}  // namespace core
}  // namespace automaton
