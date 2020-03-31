#include "automaton/core/smartproto/smart_protocol.h"

#include <fstream>
#include <string>
#include <utility>

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

smart_protocol::~smart_protocol() {
  for (schema* s : schemas) {
    delete s;
  }
}

std::shared_ptr<smart_protocol> smart_protocol::get_protocol(const std::string& proto_id) {
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

bool smart_protocol::load(const std::string& id, const std::string& path) {
  if (protocols.find(id) != protocols.end()) {
    LOG(WARNING) << "A protocol with this id( " << id << ") is already loaded! ";
    return false;
  }
  std::shared_ptr<smart_protocol> proto(new smart_protocol());
  std::ifstream i(path + "config.json");
  if (!i.is_open()) {
    LOG(WARNING) << "Error while opening " << path << "config.json";
    return false;
  } else {
    proto->config_json = std::string(std::istreambuf_iterator<char>(i), {});
    i.seekg(0);
    nlohmann::json j;
    i >> j;
    i.close();
    proto->update_time_slice = j["update_time_slice"];
    std::vector<std::string> schemas_filenames = j["schemas"];
    nlohmann::json filenames = j["files"];
    std::vector<std::string> wm = j["wire_msgs"];
    proto->wire_msgs = wm;
    for (auto cmd : j["commands"]) {
      proto->commands.push_back({cmd[0], cmd[1], cmd[2]});
    }

    for (uint32_t z = 0; z < schemas_filenames.size(); ++z) {
      std::string file_content = get_file_contents((path + schemas_filenames[z]).c_str());
      proto->msgs_defs[schemas_filenames[z]] = file_content;
      proto->schemas.push_back(new protobuf_schema(file_content));
      proto->factory->import_schema(proto->schemas.back(), "", "");
    }

    for (nlohmann::json::iterator it = filenames.begin(); it != filenames.end(); ++it) {
      std::unordered_map<std::string, std::string> extracted_files;
      std::vector<std::string> fnames = it.value();
      for (uint32_t z = 0; z < fnames.size(); ++z) {
        std::string file_name = fnames[z];
        extracted_files[file_name] = get_file_contents((path + file_name).c_str());
      }
      proto->files.insert(std::make_pair(it.key(), std::move(extracted_files)));
    }

    for (uint32_t wire_id = 0; wire_id < proto->wire_msgs.size(); ++wire_id) {
      std::string& wire_msg = proto->wire_msgs[wire_id];
      auto factory_id = proto->factory->get_schema_id(wire_msg);
      proto->factory_to_wire[factory_id] = wire_id;
      proto->wire_to_factory[wire_id] = factory_id;
    }
  }
  protocols[id] = proto;
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

std::unordered_map<std::string, std::string> smart_protocol::get_files(std::string files_type) {
  // return all files
  if (files_type == "") {
    std::unordered_map<std::string, std::string> result;
    for (auto it : files) {
      result.insert(it.second.begin(), it.second.end());
    }
    return result;
  }
  auto it = files.find(files_type);
  if (it != files.end()) {
    return it->second;
  }
  return {};
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

int32_t smart_protocol::get_wire_from_factory(int32_t msg_schema_id) {
  auto it = factory_to_wire.find(msg_schema_id);
  if (it != factory_to_wire.end()) {
    return it->second;
  }
  return -1;
}

int32_t smart_protocol::get_factory_from_wire(int32_t wire_id) {
  auto it = wire_to_factory.find(wire_id);
  if (it != wire_to_factory.end()) {
    return it->second;
  }
  return -1;
}

}  // namespace smartproto
}  // namespace core
}  // namespace automaton
