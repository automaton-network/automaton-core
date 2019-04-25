#include "automaton/core/node/lua_node/lua_node.h"

#include <memory>
#include "automaton/core/io/io.h"

using automaton::core::common::status;
using automaton::core::data::msg;
using automaton::core::data::schema;

using std::mutex;
using std::string;
using std::vector;

namespace automaton {
namespace core {
namespace node {
namespace luanode {

static std::string fresult(string fname, sol::protected_function_result pfr) {
  if (!pfr.valid()) {
    sol::error err = pfr;
    string what = err.what();
    LOG(ERROR) << "*** SCRIPT ERROR IN " << fname << "***\n" << what;
    return what;
  }
  return "";
}

lua_node::lua_node(const std::string& id, const std::string& proto_id): node(id, proto_id) {}

lua_node::~lua_node() {}

void lua_node::init() {
  std::shared_ptr<automaton::core::smartproto::smart_protocol> proto =
      automaton::core::smartproto::smart_protocol::get_protocol(protoid);
  if (!proto) {
    throw std::invalid_argument("No such protocol: " + protoid);
  }
  engine.set_factory(proto->get_factory());
  std::vector<std::string> lua_scripts;
  auto files = proto->get_files("lua_scripts");
  for (auto it : files) {
    lua_scripts.push_back(it.second);
  }
  init_bindings(proto->get_schemas(), lua_scripts, proto->get_wire_msgs(), proto->get_commands());
}

void lua_node::init_bindings(vector<schema*> schemas,
                         vector<string> lua_scripts,
                         vector<string> wire_msgs,
                         vector<string> commands) {
  engine.bind_core();

  // Bind node methods.
  engine.set_function("send",
    [this](uint32_t peer_id, msg& m, uint32_t msg_id) {
      send_message(peer_id, m, msg_id);
    });

  engine.set_function("log",
    [this](string logger, string msg) {
      // LOG(TRACE) << "[" << logger << "] " << msg;
      log(logger, msg);
    });

  engine.set_function("connect",
    [this](uint32_t peer_id) {
      log("connect_", "CONNECTED " + std::to_string(peer_id));
      connect(peer_id);
    });

  engine.set_function("disconnect",
    [this](uint32_t peer_id) {
      log("disconnect_", "DISCONNECTED " + std::to_string(peer_id));
      disconnect(peer_id);
    });

  engine["nodeid"] = nodeid;

  uint32_t script_id = 0;
  for (string lua_script : lua_scripts) {
    script_id++;
    fresult("script " + std::to_string(script_id), engine.safe_script(lua_script));
  }

  script_on_update = engine["update"];
  script_on_connected = engine["connected"];
  script_on_disconnected = engine["disconnected"];
  script_on_msg_sent = engine["sent"];
  script_on_debug_html = engine["debug_html"];

  // Map wire msg IDs to script functions.
  for (uint32_t wire_id = 0; wire_id < wire_msgs.size(); ++wire_id) {
    string wire_msg = wire_msgs[wire_id];
    string function_name = "on_" + wire_msg;
    LOG(DEBUG) << wire_id << ": " << function_name;
    script_on_msg[wire_id] = engine[function_name];
  }

  for (auto cmd : commands) {
    if (engine[cmd] != sol::lua_nil) {
      LOG(DEBUG) << "command: " << cmd;
      script_on_cmd[cmd] = engine[cmd];
    } else {
      LOG(DEBUG) << "command: " << cmd << " does not exist!";
    }
  }
}

std::string lua_node::process_cmd(const std::string& cmd, const std::string& msg) {
  if (script_on_cmd.count(cmd) != 1) {
    LOG(ERROR) << "Invalid command! : " << cmd << " (args: " << io::bin2hex(msg) << ")";
    return "";
  }
  sol::protected_function_result pfr;
  script_mutex.lock();
  if (!script_on_cmd[cmd].valid()) {
    LOG(ERROR) << "Invalid command " << cmd;
    return "";
  }
  try {
    if (msg != "") {
      LOG(INFO) << "calling script func " << cmd << " with msg " << msg;
      pfr = script_on_cmd[cmd](msg);
    } else {
      LOG(INFO) << "calling script func " << cmd << " without msg";
      pfr = script_on_cmd[cmd]();
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "Error while executing command!!! : " << e.what();
  } catch (...) {
    LOG(ERROR) << "Error while executing command!!!";
  }
  script_mutex.unlock();
  if (!pfr.valid()) {
    sol::error err = pfr;
    string what = err.what();
    LOG(ERROR) << "*** SCRIPT ERROR IN " << cmd << "***\n" << what;
    return "";
  }
  std::string result = pfr;
  return result;
}

void lua_node::script(const std::string& command, std::promise<std::string>* result) {
  add_task([this, command, result]() {
    try {
      auto pfr = engine.safe_script(command);
      if (result != nullptr) {
        result->set_value(pfr);
      }
      return "";
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
    return "";
  });
}

void lua_node::s_on_blob_received(peer_id p_id, const string& blob) {
  auto wire_id = blob[0];
  if (script_on_msg.count(wire_id) != 1) {
    LOG(FATAL) << "Invalid wire msg_id sent to us!";
    return;
  }
  msg* m = get_wire_msg(blob).release();
  add_task([this, wire_id, p_id, m]() -> string {
    try {
      auto r = fresult("on_" + m->get_message_type(), script_on_msg[wire_id](p_id, m));
      // ?? delete m;
      return r;
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
    return "";
  });
}

void lua_node::s_on_msg_sent(peer_id c, uint32_t id, const common::status& s) {
  add_task([this, c, id, s]() -> string {
    try {
      return fresult("sent", script_on_msg_sent(c, id, s.code == status::OK));
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
    return "";
  });
}

void lua_node::s_on_connected(peer_id p_id) {
  add_task([this, p_id]() -> string {
    try {
      return fresult("connected", script_on_connected(static_cast<uint32_t>(p_id)));
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
    return "";
  });
}

void lua_node::s_on_disconnected(peer_id p_id) {
  add_task([this, p_id]() -> string {
    try {
      return fresult("disconnected", script_on_disconnected(static_cast<uint32_t>(p_id)));
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    }
    return "";
  });
}

void lua_node::s_on_error(peer_id id, const std::string& message) {}

void lua_node::s_update(uint64_t time) {
  try {
    if (script_on_update.valid()) {
      fresult("update", script_on_update(time));
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
  }
}

std::string lua_node::s_debug_html() {
  try {
    auto result = script_on_debug_html();
    if (result.valid()) {
      return result;
    } else {
      return "Invalid or missing debug_html() in script.";
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
  }
  return "";
}

}  // namespace luanode
}  // namespace node
}  // namespace core
}  // namespace automaton
