#ifndef AUTOMATON_CORE_NODE_NODE_H_
#define AUTOMATON_CORE_NODE_NODE_H_

#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/data/msg.h"
#include "automaton/core/network/acceptor.h"
#include "automaton/core/network/connection.h"
#include "automaton/core/smartproto/smart_protocol.h"

namespace automaton {
namespace core {
namespace node {

typedef network::connection_id peer_id;

struct peer_info {
  peer_id id;
  std::string address;
  std::shared_ptr<network::connection> connection;
  std::shared_ptr<char> buffer;
  peer_info();
};

class node: public network::connection::connection_handler,
            public network::acceptor::acceptor_handler,
            public std::enable_shared_from_this<node> {
 public:
  typedef std::shared_ptr<node> (*factory_function)(const std::string& id, const std::string& proto_id);
  static std::shared_ptr<node> create(const std::string& type, const std::string& id, const std::string& proto_id);
  static void register_node_type(const std::string& type, factory_function func);

  static std::vector<std::string> list_nodes();
  static std::shared_ptr<node> get_node(const std::string& node_id);
  static bool launch_node(const std::string& node_type, const std::string& node_id, const std::string& protocol_id,
      const std::string& address);
  static void remove_node(const std::string& node_id);

  ~node();

  virtual void init() = 0;

  std::string get_id() const;

  std::string get_protocol_id() const;

  peer_info get_peer_info(peer_id id);

  bool set_peer_info(peer_id id, const peer_info& info);

  void send_message(peer_id id, const data::msg& msg, uint32_t msg_id);

  void send_blob(peer_id id, const std::string& blob, uint32_t msg_id);

  bool connect(peer_id id);

  bool disconnect(peer_id id);

  bool set_acceptor(const std::string& address);

  std::shared_ptr<network::acceptor> get_acceptor();

  peer_id add_peer(const std::string& address);

  void remove_peer(peer_id id);

  std::vector<peer_id> list_known_peers();

  std::set<peer_id> list_connected_peers();

  // Execute a script which returns corresponding type
  virtual void script(const std::string& command, std::promise<std::string>* result) {}

  void log(const std::string& logger, const std::string& msg);

  void dump_logs(const std::string& html_file);

  virtual std::string process_cmd(const std::string& cmd, const std::string& params) {
    return "";
  }

  void process_update(uint64_t current_time);

  uint64_t get_time_to_update();

 protected:
  node(const std::string& id, const std::string& proto_id);

  std::unique_ptr<data::msg> get_wire_msg(const std::string& blob);
  uint32_t find_message_id(const std::string& name, std::shared_ptr<data::factory> factory);
  std::unique_ptr<data::msg> create_msg_by_id(uint32_t id, std::shared_ptr<data::factory> factory);

  void add_task(std::function<std::string()> task);

  std::string nodeid;
  std::string protoid;

  std::mutex script_mutex;

 private:
  static std::map<std::string, factory_function> node_factory;
  static std::unordered_map<std::string, std::shared_ptr<node> > nodes;
  peer_id peer_ids;

  uint32_t update_time_slice;
  uint64_t time_to_update;
  std::mutex time_mutex;

  // Network
  std::shared_ptr<network::acceptor> acceptor_;
  std::mutex peers_mutex;
  std::unordered_map<peer_id, peer_info> known_peers;
  std::set<peer_id> connected_peers;
  std::mutex peer_ids_mutex;

  // Logging
  std::mutex log_mutex;
  std::unordered_map<std::string, std::vector<std::string>> logs;

  peer_id get_next_peer_id();

  bool address_parser(const std::string& s, std::string* protocol, std::string* address);

  std::mutex tasks_mutex;
  std::deque<std::function<std::string()>> tasks;

  std::shared_ptr<automaton::core::smartproto::smart_protocol> proto;

  // Inherited handlers' functions

  void on_message_received(peer_id c, std::shared_ptr<char> buffer, uint32_t bytes_read, uint32_t id);

  void on_message_sent(peer_id c, uint32_t id, const common::status& s);

  void on_connected(peer_id c);

  void on_disconnected(peer_id c);

  void on_connection_error(peer_id c, const common::status& s);

  bool on_requested(network::acceptor_id a, const std::string& address, peer_id* id);

  void on_connected(network::acceptor_id a, std::shared_ptr<network::connection> c,
      const std::string& address);

  void on_acceptor_error(network::acceptor_id a, const common::status& s);

  // Script handler functions
  virtual void s_on_blob_received(peer_id id, const std::string& blob) {}
  virtual void s_on_msg_sent(peer_id c, uint32_t id, const common::status& s) {}
  virtual void s_on_connected(peer_id id) {}
  virtual void s_on_disconnected(peer_id id) {}
  virtual void s_on_error(peer_id id, const std::string& message) {}
  virtual void s_update(uint64_t time) {}
  virtual std::string s_debug_html() = 0;
};

}  // namespace node
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NODE_NODE_H_
