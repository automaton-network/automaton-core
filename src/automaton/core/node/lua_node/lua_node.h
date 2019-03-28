#ifndef AUTOMATON_CORE_NODE_LUA_NODE_H_
#define AUTOMATON_CORE_NODE_LUA_NODE_H_

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/data/msg.h"
#include "automaton/core/data/schema.h"
#include "automaton/core/script/engine.h"
#include "automaton/core/smartproto/smart_protocol.h"

#include "automaton/core/node/node.h"

namespace automaton {
namespace core {
namespace node {
namespace luanode {

class lua_node : public node {
 public:
  lua_node(const std::string& id, const std::string& proto_id);

  ~lua_node();

  void init();

  void init_bindings(std::vector<automaton::core::data::schema*> schemas,
                     std::vector<std::string> lua_scripts,
                     std::vector<std::string> wire_msgs,
                     std::vector<std::string> commands);

  void script(const std::string& command, std::promise<std::string>* result);

  uint32_t find_message_id(const std::string& name) {
    return engine.get_factory()->get_schema_id(name);
  }

  std::unique_ptr<data::msg> create_msg_by_id(uint32_t id) {
    return engine.get_factory()->new_message_by_id(id);
  }

  std::string process_cmd(const std::string& cmd, const std::string& params);

 private:
  // Script processing related
  script::engine engine;

  sol::protected_function script_on_connected;
  sol::protected_function script_on_disconnected;
  sol::protected_function script_on_update;
  sol::protected_function script_on_msg_sent;
  std::unordered_map<uint32_t, sol::protected_function> script_on_msg;
  std::unordered_map<std::string, sol::protected_function> script_on_cmd;
  sol::protected_function script_on_debug_html;

  // Script handler functions
  void s_on_blob_received(peer_id id, const std::string& blob);
  void s_on_msg_sent(peer_id c, uint32_t id, const common::status& s);
  void s_on_connected(peer_id id);
  void s_on_disconnected(peer_id id);
  void s_on_error(peer_id id, const std::string& message);
  void s_update(uint64_t time);
  std::string s_debug_html();
};

}  // namespace luanode
}  // namespace node
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NODE_LUA_NODE_H_
