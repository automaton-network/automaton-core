#include "automaton/core/node/node.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <utility>

#include <boost/algorithm/string/replace.hpp>
#include <json.hpp>

#include "automaton/core/io/io.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"


using automaton::core::common::status;

using automaton::core::data::msg;
using automaton::core::data::schema;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;

using automaton::core::network::acceptor;
using automaton::core::network::acceptor_id;
using automaton::core::network::connection;

using std::chrono::system_clock;
using std::future;
using std::ios_base;
using std::lock_guard;
using std::make_unique;
using std::mutex;
using std::ofstream;
using std::promise;
using std::string;
using std::unique_ptr;
using std::vector;

namespace automaton {
namespace core {
namespace smartproto {

// TODO(kari): Make buffers in connection shared_ptr

static const uint32_t MAX_MESSAGE_SIZE = 1 * 1024;  // Maximum size of message in bytes
static const uint32_t HEADER_SIZE = 3;
static const uint32_t WAITING_HEADER = 1;
static const uint32_t WAITING_MESSAGE = 2;

peer_info::peer_info(): id(0), address("") {}

static std::string fresult(string fname, sol::protected_function_result pfr) {
  if (!pfr.valid()) {
    sol::error err = pfr;
    string what = err.what();
    LOG(ERROR) << "*** SCRIPT ERROR IN " << fname << "***\n" << what;
    return what;
  }

  return "";
}

void html_escape(std::string *data) {
  using boost::algorithm::replace_all;
  replace_all(*data, "&",  "&amp;");
  replace_all(*data, "\"", "&quot;");
  replace_all(*data, "\'", "&apos;");
  replace_all(*data, "<",  "&lt;");
  replace_all(*data, ">",  "&gt;");
}

std::string node::debug_html() {
  auto result = script_on_debug_html();
  if (result.valid()) {
    return result;
  } else {
    return "Invalid or missing debug_html() in script.";
  }
}

// node::node(std::string id,
//            std::string proto_id,
//            uint32_t update_time_slice,
//            vector<schema*> schemas,
//            vector<string> lua_scripts,
//            vector<string> wire_msgs,
//            vector<string> commands,
//            data::factory& factory)
//     : nodeid(id)
//     , protoid(proto_id)
//     , peer_ids(0)
//     , engine(factory)
//     , update_time_slice(update_time_slice)
//     , acceptor_(nullptr) {
//   LOG(DEBUG) << "Node constructor called";
//   init_bindings(std::move(schemas), std::move(lua_scripts), std::move(wire_msgs), std::move(commands));
//   init_worker();
// }

void node::init_bindings(vector<schema*> schemas,
                         vector<string> lua_scripts,
                         vector<string> wire_msgs,
                         vector<string> commands) {
  engine.bind_core();

  // for (auto schema : schemas) {
  //   engine.import_schema(schema);
  // }

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

  // Map wire msg IDs to factory msg IDs and vice versa.
  uint32_t wire_id = 0;
  for (auto wire_msg : wire_msgs) {
    auto factory_id = engine.get_factory()->get_schema_id(wire_msg);
    factory_to_wire[factory_id] = wire_id;
    wire_to_factory[wire_id] = factory_id;
    string function_name = "on_" + wire_msg;
    LOG(DEBUG) << wire_id << ": " << function_name;
    script_on_msg[wire_id] = engine[function_name];
    wire_id++;
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

void node::init_worker() {
  lock_guard<mutex> lock(worker_mutex);
  worker_stop_signal = false;
  worker = new std::thread([this]() {
    // LOG(DEBUG) << "Worker thread starting in " << nodeid;
    while (!get_worker_stop_signal()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(this->update_time_slice));
      auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::system_clock::now().time_since_epoch()).count();

      // Call update function only if a valid definition for it exists.
      if (script_on_update.valid()) {
        // LOG(DEBUG) << "Calling update in " << nodeid;
        script_mutex.lock();
        fresult("update", script_on_update(current_time));
        script_mutex.unlock();
      }

      // TODO(asen): custom break condition, e.g. max number of tasks per update.
      // Process tasks pending in the queue.
      // LOG(DEBUG) << "Executing tasks in " << nodeid;
      while (!tasks.empty()) {
        // Check to see if we should stop the thread execution.
        if (get_worker_stop_signal()) {
          break;
        }

        // Pop a task from the front of the queue.
        tasks_mutex.lock();
        auto task = tasks.front();
        tasks.pop_front();
        tasks_mutex.unlock();

        // Execute task.
        // LOG(DEBUG) << "  - Executing a task in " << nodeid;
        script_mutex.lock();
        try {
          string result = task();
          if (result.size() > 0) {
            engine.script("for k,v in pairs(_G) do print(k .. ' = ' .. tostring(v)) end");
            LOG(FATAL) << "TASK FAILED: " << result;
          }
        } catch (const std::exception& ex) {
          LOG(FATAL) << "TASK FAILED: EXCEPTION1: " << ex.what();
        } catch (std::string s) {
          LOG(FATAL) << "TASK FAILED: EXCEPTION2: " << s;
        } catch (...) {
          LOG(FATAL) << "TASK FAILED: EXCEPTION DURING TASK EXECUTION!";
        }
        script_mutex.unlock();
      }
    }
  });
}
node::node(const std::string& id,
           std::string proto_id):
           // data::factory& factory):
      nodeid(id)
    , protoid(proto_id)
    , peer_ids(0)
    // , engine(factory)
    , acceptor_(nullptr) {
  LOG(DEBUG) << "Node constructor called";
  std::shared_ptr<smart_protocol> proto = smart_protocol::get_protocol(proto_id);
  update_time_slice = proto->get_update_time_slice();
  engine.set_factory(proto->get_factory());
  init_bindings(proto->get_schemas(), proto->get_scripts(), proto->get_wire_msgs(), proto->get_commands());
  init_worker();
}

node::~node() {
  // LOG(DEBUG) << "Node destructor called";
  //
  // vector<peer_id> res = list_known_peers();
  // LOG(DEBUG) << "Known peers " << res.size();
  // for (uint32_t i = 0; i < res.size(); ++i) {
  //   LOG(DEBUG) << "known_peer: " << res[i];
  // }

  worker_mutex.lock();
  worker_stop_signal = true;
  worker_mutex.unlock();

  worker->join();
  delete worker;
}

bool node::get_worker_stop_signal() {
  lock_guard<mutex> lock(worker_mutex);
  return worker_stop_signal;
}

// TODO(kari): move to io?
static string get_date_string(system_clock::time_point t) {
  auto as_time_t = std::chrono::system_clock::to_time_t(t);
  struct tm* tm;
  if ((tm = ::gmtime(&as_time_t))) {
    char some_buffer[64];
    if (std::strftime(some_buffer, sizeof(some_buffer), "%F %T", tm)) {
      return string{some_buffer};
    }
  }
  throw std::runtime_error("Failed to get current date as string");
}

// TODO(kari): move to io?
string zero_padded(int num, int width) {
  std::ostringstream ss;
  ss << std::setw(width) << std::setfill('0') << num;
  return ss.str();
}

std::string node::get_id() {
  return nodeid;
}

std::string node::get_protocol_id() {
  return protoid;
}

peer_info node::get_peer_info(peer_id pid) {
  auto it = known_peers.find(pid);
  if (it == known_peers.end()) {
    return peer_info();
  }
  return it->second;
}

void node::log(string logger, string msg) {
  lock_guard<mutex> lock(log_mutex);

  // LOG(TRACE) << "[" << logger << "] " << msg;
  if (logs.count(logger) == 0) {
    logs.emplace(logger, vector<string>());
  }
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
     now.time_since_epoch()).count();
  logs[logger].push_back(
    "[" + get_date_string(now) + "." + zero_padded(current_time % 1000, 3) + "] " + msg);
}

void node::dump_logs(string html_file) {
  add_task([this, html_file](){
    ofstream f;
    f.open(html_file, ios_base::trunc);
    if (!f.is_open()) {
      LOG(ERROR) << "Error while opening " << html_file;
      return "";
    }
    f << R"(
<html>
<head>
<meta charset="utf-8"/>

<script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/vis/4.21.0/vis.min.js"></script>
<link href="https://cdnjs.cloudflare.com/ajax/libs/vis/4.21.0/vis.min.css" rel="stylesheet" type="text/css" />

<style type="text/css">
  #mynetwork {
    width: 1000px;
    height: 600px;
    border: 1px solid lightgray;
  }

  pre {
    border: 1px solid black;
    padding: 8px;
    overflow:auto;
    font-size: 16px;
    font-family: 'Inconsolata', monospace;
  }

  .button {
    font: bold 11px Play;
    text-decoration: none;
    background-color: #aad8f3;
    color: #333;
    padding: 2px 6px 2px 6px;
    border-top: 1px solid #CCCCCC;
    border-right: 1px solid #333333;
    border-bottom: 1px solid #333333;
    border-left: 1px solid #CCCCCC;
    margin: 4px;
    line-height: 24px;
  }
</style>
</head>
<body>
<br/>
)";

    log_mutex.lock();
    for (auto log : logs) {
      f << "<a class='button' href='#" << log.first << "'>";
      f << log.first << std::endl;
      f << "</a>\n";
    }
    log_mutex.unlock();

    f << "<hr />\n";
    f << debug_html();
    f << "<hr />\n";

    log_mutex.lock();
    for (auto log : logs) {
      f << "<br/><span class='button' id='" << log.first << "'>" << log.first << "</span>";
      f << "<pre>";
      for (auto msg : log.second) {
        html_escape(&msg);
        f << msg << "\n";
      }
      f << "</pre>\n";
    }
    log_mutex.unlock();

    f << "</body></html>\n";
    f.close();

    return "";
  });
}

void node::send_message(peer_id p_id, const core::data::msg& msg, uint32_t msg_id) {
  auto msg_schema_id = msg.get_schema_id();
  CHECK_GT(factory_to_wire.count(msg_schema_id), 0)
      << "Message " << msg.get_message_type()
      << " not part of the protocol";
  auto wire_id = factory_to_wire[msg_schema_id];
  string msg_blob;
  if (msg.serialize_message(&msg_blob)) {
    msg_blob.insert(0, 1, static_cast<char>(wire_id));
    send_blob(p_id, msg_blob, msg_id);
  } else {
    LOG(DEBUG) << "Could not serialize message!";
  }
}

void node::s_on_blob_received(peer_id p_id, const string& blob) {
  auto wire_id = blob[0];
  if (script_on_msg.count(wire_id) != 1) {
    LOG(FATAL) << "Invalid wire msg_id sent to us!";
    return;
  }
  CHECK_GT(wire_to_factory.count(wire_id), 0);
  auto msg_id = wire_to_factory[wire_id];
  msg* m = engine.get_factory()->new_message_by_id(msg_id).release();
  m->deserialize_message(blob.substr(1));
  add_task([this, wire_id, p_id, m, blob]() -> string {
    auto r = fresult("on_" + m->get_message_type(), script_on_msg[wire_id](p_id, m));
    // delete m;
    return r;
  });
}

std::string node::process_cmd(std::string cmd, std::string msg) {
  if (script_on_cmd.count(cmd) != 1) {
    LOG(ERROR) << "Invalid command! : " << cmd << " (args: " << io::bin2hex(msg) << ")";
    return "";
  }
  sol::protected_function_result pfr;
  script_mutex.lock();
  try {
    if (msg != "") {
      std::cout << "calling script func " << cmd << " with msg " << msg << std::endl;
      pfr = script_on_cmd[cmd](msg);
    } else {
      std::cout << "calling script func " << cmd << " without msg" << std::endl;
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
  std::cout << "RESULT: " << io::bin2hex(result) << std::endl;
  return result;
}

void node::send_blob(peer_id p_id, const string& blob, uint32_t msg_id) {
  LOG(DEBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") << " sending message " << core::io::bin2hex(blob) <<
      " to peer " << p_id;
  uint32_t blob_size = blob.size();
  if (blob_size > MAX_MESSAGE_SIZE) {
    LOG(ERROR) << "Message size is " << blob_size << " and is too big! Max message size is " << MAX_MESSAGE_SIZE;
    add_task([this, p_id, msg_id]() -> string {
      return fresult("sent", script_on_msg_sent(p_id, msg_id, false));
    });
    return;
  }
  char buffer[3];
  buffer[2] = blob_size & 0xff;
  buffer[1] = (blob_size >> 8) & 0xff;
  buffer[0] = (blob_size >> 16) & 0xff;
  // TODO(kari): Find more effective way to do this
  string new_message = string(buffer, 3) + blob;
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  lock_guard<mutex> lock(peers_mutex);
  if (connected_peers.find(p_id) == connected_peers.end()) {
    LOG(ERROR) << "Peer " << p_id << " is not connected! Call connect first!";
    add_task([this, p_id, msg_id]() -> string {
      return fresult("sent", script_on_msg_sent(p_id, msg_id, false));
    });
    return;
  }
  auto it = known_peers.find(p_id);
  if (it == known_peers.end()) {
    LOG(ERROR) << "Trying to send message to unknown peer " << p_id;
    add_task([this, p_id, msg_id]() -> string {
      return fresult("sent", script_on_msg_sent(p_id, msg_id, false));
    });
    return;
  }
  if (it->second.connection) {
    if (it->second.connection->get_state() == connection::state::connected) {
      it->second.connection->async_send(new_message, msg_id);
    }
  } else {
    LOG(ERROR) << "No connection in peer " << p_id;
    add_task([this, p_id, msg_id]() -> string {
      return fresult("sent", script_on_msg_sent(p_id, msg_id, false));
    });
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
}

void node::s_on_connected(peer_id p_id) {
  add_task([this, p_id]() -> string {
    return fresult("connected", script_on_connected(static_cast<uint32_t>(p_id)));
  });
}

void node::s_on_disconnected(peer_id p_id) {
  add_task([this, p_id]() -> string {
    return fresult("disconnected", script_on_disconnected(static_cast<uint32_t>(p_id)));
  });
}

bool node::connect(peer_id p_id) {
// VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  lock_guard<mutex> lock(peers_mutex);
  if (connected_peers.find(p_id) != connected_peers.end()) {
    LOG(DEBUG) << "Peer " << p_id << " is already connected!";
    // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
    return false;
  }
  auto it = known_peers.find(p_id);
  if (it != known_peers.end()) {
    if (!it->second.connection) {
      LOG(ERROR) << "Connection does not exist!";
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
      return false;
    }
    if (it->second.connection->get_state() == connection::state::disconnected) {
      it->second.connection->connect();
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
      return true;
    }
  } else {
    LOG(ERROR) << "No such peer " << p_id;
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  return false;
}

bool node::disconnect(peer_id p_id) {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  peers_mutex.lock();
  auto it1 = connected_peers.find(p_id);
  if (it1 != connected_peers.end()) {
    auto it = known_peers.find(p_id);
    if (it != known_peers.end()) {
      std::shared_ptr<connection> connection = (it->second.connection);
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
      peers_mutex.unlock();
      connection->disconnect();
      return true;
    } else {
      // not in known peers
    }
  } else {
    LOG(ERROR) << "Peer " << p_id << " is not connected!";
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  peers_mutex.unlock();
  return false;
}

bool node::set_acceptor(const char* address) {
  std::shared_ptr<acceptor> new_acceptor;
  try {
    string protocol, addr;
    if (!address_parser(address, &protocol, &addr)) {
      LOG(DEBUG) << "Address was not parsed!";
      return false;
    }
    new_acceptor = std::shared_ptr<acceptor>(acceptor::create(protocol, 1, addr, this, this));
    if (new_acceptor && !new_acceptor->init()) {
      LOG(DEBUG) << "Acceptor initialization failed! Acceptor was not created!" << address;
      return false;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Adding acceptor failed! " << address << " Error: " << e.what();
    return false;
  }
  if (!new_acceptor) {
    LOG(ERROR) << "Acceptor was not created!";
    return false;
  }
  acceptor_ = new_acceptor;
  new_acceptor->start_accepting();
  return true;
}

std::shared_ptr<network::acceptor> node::get_acceptor() {
  return acceptor_;
}

peer_id node::add_peer(const string& address) {
  // TODO(kari): Return 0 on error?
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  lock_guard<mutex> lock(peers_mutex);
  for (auto it = known_peers.begin(); it != known_peers.end(); ++it) {
    if (it->second.address == address) {
      LOG(ERROR) << "Already have peer " << address;
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
      return it->first;
    }
  }
  peer_info info;
  info.address = address;
  info.id = get_next_peer_id();
  info.connection = nullptr;
  info.buffer = std::shared_ptr<char>(new char[MAX_MESSAGE_SIZE], std::default_delete<char[]>());
  std::shared_ptr<connection> new_connection;
  try {
    string protocol, addr;
    if (!address_parser(address, &protocol, &addr)) {
      LOG(DEBUG) << "Address was not parsed! " << address;
    } else {
      new_connection = std::shared_ptr<connection>
          (connection::create(protocol, info.id, addr, this));
      if (new_connection && !new_connection->init()) {
        LOG(DEBUG) << "Connection initialization failed! Connection was not created!";
      }
    }
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
  }
  if (!new_connection) {
    LOG(DEBUG) << "No new connection";
  } else {
    info.connection = new_connection;
  }
  known_peers[info.id] = std::move(info);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  return info.id;
}

void node::remove_peer(peer_id p_id) {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  peers_mutex.lock();
  auto it1 = known_peers.find(p_id);
  if (it1 != known_peers.end()) {
    auto it2 = connected_peers.find(p_id);
    if (it2 != connected_peers.end()) {
      std::shared_ptr<connection> connection = (it1->second.connection);
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
      peers_mutex.unlock();
      connection->disconnect();
      // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
      peers_mutex.lock();
    }
    known_peers.erase(it1);
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  peers_mutex.unlock();
}

vector<peer_id> node::list_known_peers() {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  lock_guard<mutex> lock(peers_mutex);
  vector<peer_id> res;
  for (auto it = known_peers.begin(); it != known_peers.end(); ++it) {
    res.push_back(it->first);
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  return res;
}

std::set<peer_id> node::list_connected_peers() {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  lock_guard<mutex> lock(peers_mutex);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  return connected_peers;
}

peer_id node::get_next_peer_id() {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  lock_guard<mutex> lock(peer_ids_mutex);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
  return ++peer_ids;
}

void node::script(std::string command, std::promise<std::string>* result) {
  add_task([this, command, result]() {
    auto pfr = engine.safe_script(command);
    if (result != nullptr) {
      result->set_value(pfr);
    }
    return "";
  });
}

bool node::address_parser(const string& s, string* protocol, string* address) {
  std::regex rgx_ip("(.+)://(.+)");
  std::smatch match;
  if (std::regex_match(s.begin(), s.end(), match, rgx_ip) &&
      match.size() == 3) {
    *protocol = match[1];
    *address = match[2];
    return true;
  } else {
    LOG(DEBUG) << "match size: " << match.size();
    for (uint32_t i = 0; i < match.size(); i++) {
      LOG(DEBUG) << "match " << i << " -> " << match[i];
    }
    *protocol = "";
    *address = "";
    return false;
  }
}

void node::on_message_received(peer_id c, char* buffer, uint32_t bytes_read, uint32_t mid) {
  LOG(DEBUG) << "RECEIVED: " << core::io::bin2hex(string(buffer, bytes_read)) << " from peer " << c;
  switch (mid) {
    case WAITING_HEADER: {
      if (bytes_read != HEADER_SIZE) {
        LOG(ERROR) << "Wrong header size received";
        s_on_error(c, "Wrong header size received");
        disconnect(c);
        return;
      }
      // TODO(kari): check if this peer still exists, the buffer could be invalid
      // TODO(kari): make this loop
      uint32_t message_size = 0;
      message_size += (buffer[2] & 0x000000ff);
      message_size += ((buffer[1] & 0x000000ff) << 8);
      message_size += ((buffer[0] & 0x000000ff) << 16);
      LOG(DEBUG) << "MESSAGE SIZE: " << message_size;
      if (!message_size || message_size > MAX_MESSAGE_SIZE) {
        LOG(ERROR) << "Invalid message size!";
        s_on_error(c, "Invalid message size!");
        disconnect(c);
        return;
      } else {
        // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
        peers_mutex.lock();
        auto it = known_peers.find(c);
        if (it != known_peers.end() && it->second.connection && it->second.connection->get_state() ==
            connection::state::connected) {
          auto connection_ = it->second.connection;
          LOG(DEBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") << " waits message with size " << message_size
              << " from peer " << c;
          // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
          peers_mutex.unlock();
          connection_->async_read(buffer, MAX_MESSAGE_SIZE, message_size, WAITING_MESSAGE);
        } else {
        // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
        peers_mutex.unlock();
        s_on_error(c, "No such peer or peer disconnected!");
        disconnect(c);
        }
      }
    }
    break;
    case WAITING_MESSAGE: {
      string blob = string(buffer, bytes_read);
      // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
      peers_mutex.lock();
      auto it = known_peers.find(c);
      if (it != known_peers.end() && it->second.connection && it->second.connection->get_state() ==
          connection::state::connected) {
        LOG(DEBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") << " received message " <<
            core::io::bin2hex(blob) << " from peer " << c;
        // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
        peers_mutex.unlock();
        it->second.connection->async_read(buffer, MAX_MESSAGE_SIZE, HEADER_SIZE, WAITING_HEADER);
        s_on_blob_received(c, blob);
      } else {
        // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
        peers_mutex.unlock();
        s_on_error(c, "No such peer or peer disconnected!");
        disconnect(c);
      }
    }
    break;
    default: {}
  }
}

void node::on_message_sent(peer_id c, uint32_t id, const common::status& s) {
  LOG(DEBUG) << "Message to peer " << c << " with msg_id " << id << " was sent " <<
      (s.code == status::OK ? "successfully" : "unsuccessfully");
  add_task([this, c, id, s]() -> string {
    return fresult("sent", script_on_msg_sent(c, id, s.code == status::OK));
  });
}

void node::on_connected(peer_id c) {
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c;
  peers_mutex.lock();
  auto it = known_peers.find(c);
  if (it == known_peers.end()) {
    LOG(ERROR) << "Connected to unknown peer " << c << " THIS SHOULD NEVER HAPPEN";
    // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c
        // << (it->second.address);
    peers_mutex.unlock();
    return;
  }
  LOG(DEBUG) << "Connected to " << c;
  connected_peers.insert(c);
  it->second.connection->async_read(it->second.buffer.get(), MAX_MESSAGE_SIZE, HEADER_SIZE, WAITING_HEADER);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c
      // << (it->second.address);
  peers_mutex.unlock();
  s_on_connected(c);
}

void node::on_disconnected(peer_id c) {
  LOG(DEBUG) << c << " -> on_disconnected";
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c;
  peers_mutex.lock();
  auto it = connected_peers.find(c);
  // If the address is not the id
  if (it != connected_peers.end()) {
    connected_peers.erase(it);
    // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c;
    peers_mutex.unlock();
    s_on_disconnected(c);
  } else {
    LOG(ERROR) << "No such peer " << c;
    // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c;
    peers_mutex.unlock();
  }
}

void node::on_connection_error(peer_id c, const common::status& s) {
  LOG(DEBUG) << c << " -> on_error " << s;
  std::stringstream ss;
  ss << "Connection error:: " << s;
  s_on_error(c, ss.str());
  disconnect(c);
}

bool node::on_requested(acceptor_id a, const string& address, peer_id* id) {
  LOG(DEBUG) << "Requested connection to " << acceptor_->get_address() << " from " << address;
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  lock_guard<mutex> lock(peers_mutex);
  for (auto it = known_peers.begin(); it != known_peers.end(); ++it) {
    if (it->second.address == address) {
      LOG(ERROR) << "Already have peer " << address;
      // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
      return false;
    }
  }
  *id = get_next_peer_id();
  peer_info info;
  info.address = address;
  info.id = *id;
  info.connection = nullptr;
  info.buffer = std::shared_ptr<char>(new char[MAX_MESSAGE_SIZE], std::default_delete<char[]>());
  known_peers[*id] = std::move(info);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  return true;
}

void node::on_connected(acceptor_id a, std::shared_ptr<network::connection> c, const string& address) {
  peer_id id = c->get_id();
  LOG(DEBUG) << "Connected in acceptor " << acceptor_->get_address() << " peer with id " <<
      id << " (" << address << ')';
  // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  peers_mutex.lock();
  auto it = known_peers.find(id);
  if (it == known_peers.end()) {
    // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
    peers_mutex.unlock();
    LOG(ERROR) << "Connected to unknown peer " << id << " (" << address << ')' << " THIS SHOULD NEVER HAPPEN";
    return;
  }
  it->second.connection = std::shared_ptr<network::connection> (c);
  LOG(DEBUG) << "Connected to " << address << " now " << c->get_address();
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  peers_mutex.unlock();
}

void node::on_acceptor_error(acceptor_id a, const common::status& s)  {
  LOG(DEBUG) << acceptor_->get_address() << " -> on_error in acceptor:: " << s;
}

}  // namespace smartproto
}  // namespace core
}  // namespace automaton
