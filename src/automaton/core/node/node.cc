#include "automaton/core/node/node.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <utility>

#include <boost/algorithm/string/replace.hpp>

#include "automaton/core/io/io.h"


using automaton::core::common::status;

using automaton::core::data::msg;

using automaton::core::network::acceptor;
using automaton::core::network::acceptor_id;
using automaton::core::network::connection;

using std::chrono::system_clock;
using std::ios_base;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::ofstream;
using std::string;
using std::vector;

// TODO(kari): Remove comments or change logging level.

namespace automaton {
namespace core {
namespace node {

static const uint32_t MAX_MESSAGE_SIZE = 1 * 1024;  // Maximum size of message in bytes
static const uint32_t HEADER_SIZE = 3;
static const uint32_t WAITING_HEADER = 1;
static const uint32_t WAITING_MESSAGE = 2;

std::unordered_map<string, std::shared_ptr<node> > node::nodes;

vector<string> node::list_nodes() {
  vector<string> result;
  for (const auto& n : nodes) {
    result.push_back(n.first);
  }
  return result;
}

std::shared_ptr<node> node::get_node(const string& node_id) {
  const auto& n = nodes.find(node_id);
  if (n != nodes.end()) {
    return n->second;
  }
  return nullptr;
}

bool node::launch_node(const string& node_type, const string& node_id, const string& protocol_id,
    const string& address) {
  auto n = nodes.find(node_id);
  if (n == nodes.end()) {
    std::shared_ptr<node> new_node = create(node_type, node_id, protocol_id);
    if (new_node == nullptr) {
      LOG(ERROR) << "Creating node failed!";
      return false;
    }
    bool res = new_node->set_acceptor(address);
    if (!res) {
      LOG(ERROR) << "Setting acceptor at address " << address << " failed!";
      std::cout << "!!! set acceptor failed" << std::endl;
      return false;
    }
    nodes[node_id] = std::move(new_node);
  } else {
    return false;
  }
  return true;
}

void node::remove_node(const string& id) {
  auto it = nodes.find(id);
  if (it != nodes.end()) {
    auto node = it->second;
    // Actions to prevent other threads (worker threads) from calling non-existent functions
    node->acceptor_->stop_accepting();
    node->acceptor_ = nullptr;
    for (auto peer = node->known_peers.begin(); peer != node->known_peers.end(); ++peer) {
      peer->second.connection->disconnect();
    }
    node->known_peers.clear();
    nodes.erase(it);
  }
}

std::shared_ptr<node> node::create(const std::string& type, const std::string& id, const std::string& proto_id) {
  auto it = node_factory.find(type);
  if (it == node_factory.end()) {
    return nullptr;
  } else {
    auto new_node = it->second(id, proto_id);
    new_node->init();
    return new_node;
  }
}

void node::register_node_type(const std::string& type, factory_function func) {
  node_factory[type] = func;
}

std::map<std::string, node::factory_function> node::node_factory;

peer_info::peer_info(): id(0), address("") {}

void html_escape(string *data) {
  using boost::algorithm::replace_all;
  replace_all(*data, "&",  "&amp;");
  replace_all(*data, "\"", "&quot;");
  replace_all(*data, "\'", "&apos;");
  replace_all(*data, "<",  "&lt;");
  replace_all(*data, ">",  "&gt;");
}

node::node(const string& id,
           const string& proto_id):
      nodeid(id)
    , protoid(proto_id)
    , peer_ids(0)
    , time_to_update(0)
    , acceptor_(nullptr) {
  // LOG(DBUG) << "Node constructor called";
  std::shared_ptr<automaton::core::smartproto::smart_protocol> proto =
      automaton::core::smartproto::smart_protocol::get_protocol(proto_id);
  if (!proto) {
    throw std::invalid_argument("No such protocol: " + proto_id);
  }
  update_time_slice = proto->get_update_time_slice();

  auto factory = proto->get_factory();
  std::vector<std::string> wire_msgs = proto->get_wire_msgs();
  for (uint32_t wire_id = 0; wire_id < wire_msgs.size(); ++wire_id) {
    string wire_msg = wire_msgs[wire_id];
    auto factory_id = factory->get_schema_id(wire_msg);
    factory_to_wire[factory_id] = wire_id;
    wire_to_factory[wire_id] = factory_id;
  }
}

node::~node() {}

std::unique_ptr<msg> node::get_wire_msg(const std::string& blob) {
  auto wire_id = blob[0];
  CHECK_GT(wire_to_factory.count(wire_id), 0);
  auto msg_id = wire_to_factory[wire_id];
  static std::shared_ptr<data::factory> factory =
      automaton::core::smartproto::smart_protocol::get_protocol(protoid)->get_factory();
  std::unique_ptr<msg> m = factory->new_message_by_id(msg_id);
  m->deserialize_message(blob.substr(1));
  return m;
}

uint32_t node::find_message_id(const std::string& name, std::shared_ptr<data::factory> factory) {
  return factory->get_schema_id(name);
}

std::unique_ptr<data::msg> node::create_msg_by_id(uint32_t id, std::shared_ptr<data::factory> factory) {
  return factory->new_message_by_id(id);
}

void node::add_task(std::function<std::string()> task) {
  std::lock_guard<std::mutex> lock(tasks_mutex);
  tasks.push_back(task);
}

string node::get_id() const {
  return nodeid;
}

string node::get_protocol_id() const {
  return protoid;
}

peer_info node::get_peer_info(peer_id pid) {
  auto it = known_peers.find(pid);
  if (it == known_peers.end()) {
    return peer_info();
  }
  return it->second;
}

void node::log(const string& logger, const string& msg) {
  lock_guard<mutex> lock(log_mutex);

  // LOG(TRACE) << "[" << logger << "] " << msg;
  if (logs.count(logger) == 0) {
    logs.emplace(logger, vector<string>());
  }
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
     now.time_since_epoch()).count();
  std::stringstream ss;
  ss << "[" << io::get_date_string(now) << "." << io::zero_padded(current_time % 1000, 3) << "] " << msg;
  logs[logger].push_back(ss.str());
}

void node::dump_logs(const string& html_file) {
  ofstream f;
  f.open(html_file, ios_base::trunc);
  if (!f.is_open()) {
    LOG(ERROR) << "Error while opening " << html_file;
    return;
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
  f << s_debug_html();
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
}

void node::process_update(uint64_t current_time) {
  time_mutex.lock();
  time_to_update = current_time + update_time_slice;
  time_mutex.unlock();
  script_mutex.lock();
  s_update(current_time);
  script_mutex.unlock();
  while (true) {
    tasks_mutex.lock();
    if (tasks.empty()) {
      tasks_mutex.unlock();
      break;
    }
    auto task = tasks.front();
    tasks.pop_front();
    tasks_mutex.unlock();
    script_mutex.lock();
    try {
      string result = task();
      if (result.size() > 0) {
        LOG(FATAL) << "TASK FAILED: " << result;
      }
    } catch (const std::exception& ex) {
      LOG(FATAL) << "TASK FAILED: EXCEPTION1: " << ex.what();
    } catch (string s) {
      LOG(FATAL) << "TASK FAILED: EXCEPTION2: " << s;
    } catch (...) {
      LOG(FATAL) << "TASK FAILED: EXCEPTION DURING TASK EXECUTION!";
    }
    script_mutex.unlock();
  }
}

uint64_t node::get_time_to_update() {
  lock_guard<mutex> lock(time_mutex);
  return time_to_update;
}

void node::send_message(peer_id p_id, const core::data::msg& msg, uint32_t msg_id) {
  auto msg_schema_id = msg.get_schema_id();
  CHECK_GT(factory_to_wire.count(msg_schema_id), 0)
      << "Message " << msg.get_message_type() << " not part of the protocol";
  auto wire_id = factory_to_wire[msg_schema_id];
  string msg_blob;
  if (msg.serialize_message(&msg_blob)) {
    msg_blob.insert(0, 1, static_cast<char>(wire_id));
    send_blob(p_id, msg_blob, msg_id);
  } else {
    LOG(ERROR) << "Could not serialize message!";
  }
}

void node::send_blob(peer_id p_id, const string& blob, uint32_t msg_id) {
  // LOG(DBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") <<
      // " sending message " << core::io::bin2hex(blob) << " to peer " << p_id;
  uint32_t blob_size = blob.size();
  if (blob_size > MAX_MESSAGE_SIZE) {
    LOG(ERROR) << "Message size is " << blob_size << " and is too big! Max message size is " << MAX_MESSAGE_SIZE;
    on_message_sent(p_id, msg_id, common::status::failed_precondition("Message size is too big!"));
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
    on_message_sent(p_id, msg_id, common::status::canceled("Peer is not connected!"));
    return;
  }
  auto it = known_peers.find(p_id);
  if (it == known_peers.end()) {
    LOG(ERROR) << "Trying to send message to unknown peer " << p_id;
    on_message_sent(p_id, msg_id, common::status::failed_precondition("Unknown peer!"));
    return;
  }
  if (it->second.connection) {
    if (it->second.connection->get_state() == connection::state::connected) {
      it->second.connection->async_send(new_message, msg_id);
    }
  } else {
    LOG(ERROR) << "No connection in peer " << p_id;
    on_message_sent(p_id, msg_id, common::status::not_found("Not connected!"));
  }
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
}

bool node::connect(peer_id p_id) {
// VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
  lock_guard<mutex> lock(peers_mutex);
  if (connected_peers.find(p_id) != connected_peers.end()) {
    LOG(WARNING) << "Peer " << p_id << " is already connected!";
    VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << p_id;
    return false;
  }
  auto it = known_peers.find(p_id);
  if (it != known_peers.end()) {
    if (it->second.connection == nullptr) {
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

bool node::set_acceptor(const string& address) {
  std::shared_ptr<acceptor> new_acceptor;
  try {
    string protocol, addr;
    if (!address_parser(address, &protocol, &addr)) {
      LOG(ERROR) << "Address was not parsed!";
      return false;
    }
    auto self = shared_from_this();
    new_acceptor = std::shared_ptr<acceptor>(acceptor::create(protocol, 1, addr, self, self));
    if (new_acceptor && !new_acceptor->init()) {
      LOG(ERROR) << "Acceptor initialization failed! Acceptor was not created! " << address;
      return false;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Adding acceptor failed! " << address << " Error: " << e.what();
    return false;
  }
  if (new_acceptor == nullptr) {
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
      LOG(ERROR) << "Address was not parsed! " << address;
    } else {
      std::shared_ptr<node> self = shared_from_this();
      new_connection = std::shared_ptr<connection>(connection::create(protocol, info.id, addr, self));
      if (new_connection && !new_connection->init()) {
        LOG(ERROR) << "Connection initialization failed! Connection was not created!";
      }
    }
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
  }
  if (new_connection == nullptr) {
    LOG(WARNING) << "No new connection";
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

bool node::address_parser(const string& s, string* protocol, string* address) {
  std::regex rgx_ip("(.+)://(.+)");
  std::smatch match;
  if (std::regex_match(s.begin(), s.end(), match, rgx_ip) &&
      match.size() == 3) {
    *protocol = match[1];
    *address = match[2];
    return true;
  } else {
    LOG(ERROR) << "match size: " << match.size();
    for (uint32_t i = 0; i < match.size(); i++) {
      LOG(ERROR) << "match " << i << " -> " << match[i];
    }
    *protocol = "";
    *address = "";
    return false;
  }
}

void node::on_message_received(peer_id c, std::shared_ptr<char> buffer, uint32_t bytes_read, uint32_t mid) {
  // LOG(DBUG) << "RECEIVED: " << core::io::bin2hex(string(buffer.get(), bytes_read)) << " from peer " << c;
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
      message_size += (buffer.get()[2] & 0x000000ff);
      message_size += ((buffer.get()[1] & 0x000000ff) << 8);
      message_size += ((buffer.get()[0] & 0x000000ff) << 16);
      // LOG(DBUG) << "MESSAGE SIZE: " << message_size;
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
          // LOG(DBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") << " waits message with size " << message_size
          //     << " from peer " << c;
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
      string blob = string(buffer.get(), bytes_read);
      // VLOG(9) << "LOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A");
      peers_mutex.lock();
      auto it = known_peers.find(c);
      if (it != known_peers.end() && it->second.connection && it->second.connection->get_state() ==
          connection::state::connected) {
        // LOG(DBUG) << (acceptor_ ? acceptor_->get_address() : "N/A") << " received message " <<
        //     core::io::bin2hex(blob) << " from peer " << c;
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
  // LOG(DBUG) << "Message to peer " << c << " with msg_id " << id << " was sent " <<
  //     (s.code == status::OK ? "successfully" : "unsuccessfully");
  s_on_msg_sent(c, id, s);
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
  // LOG(DBUG) << "Connected to " << c;
  connected_peers.insert(c);
  it->second.connection->async_read(it->second.buffer, MAX_MESSAGE_SIZE, HEADER_SIZE, WAITING_HEADER);
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " peer " << c
      // << (it->second.address);
  peers_mutex.unlock();
  s_on_connected(c);
}

void node::on_disconnected(peer_id c) {
  // LOG(DBUG) << c << " -> on_disconnected";
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
  LOG(DBUG) << c << " -> on_error " << s;
  std::stringstream ss;
  ss << "Connection error:: " << s;
  s_on_error(c, ss.str());
  disconnect(c);
}

bool node::on_requested(acceptor_id a, const string& address, peer_id* id) {
  // LOG(DBUG) << "Requested connection to " << acceptor_->get_address() << " from " << address;
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
  // LOG(DBUG) << "Connected in acceptor " << acceptor_->get_address() << " peer with id " <<
  //     id << " (" << address << ')';
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
  // LOG(DBUG) << "Connected to " << address << " now " << c->get_address();
  // VLOG(9) << "UNLOCK " << this << " " << (acceptor_ ? acceptor_->get_address() : "N/A") << " addr " << address;
  peers_mutex.unlock();
}

void node::on_acceptor_error(acceptor_id a, const common::status& s)  {
  LOG(DBUG) << acceptor_->get_address() << " -> on_error in acceptor:: " << s;
}

}  // namespace node
}  // namespace core
}  // namespace automaton
