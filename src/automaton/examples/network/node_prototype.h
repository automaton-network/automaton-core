#ifndef AUTOMATON_EXAMPLES_NETWORK_NODE_PROTOTYPE_H_
#define AUTOMATON_EXAMPLES_NETWORK_NODE_PROTOTYPE_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "automaton/core/network/simulated_connection.h"

/// Class node
class node {
 public:
  class handler: public automaton::core::network::connection::connection_handler {
   public:
    node* node_;
    explicit handler(node* n);
    void on_message_received(automaton::core::network::connection* c, char* buffer,
        uint32_t bytes_read, uint32_t id);
    void on_message_sent(automaton::core::network::connection* c, uint32_t id,
        automaton::core::network::connection::error e);
    void on_connected(automaton::core::network::connection* c);
    void on_disconnected(automaton::core::network::connection* c);
    void on_error(automaton::core::network::connection* c,
        automaton::core::network::connection::error e);
  };
  class lis_handler: public automaton::core::network::acceptor::acceptor_handler {
   public:
    node* node_;
    explicit lis_handler(node* n);
    bool on_requested(automaton::core::network::acceptor* a, const std::string& address);
    void on_connected(automaton::core::network::acceptor* a,
        automaton::core::network::connection* c, const std::string& address);
    void on_error(automaton::core::network::acceptor* a,
        automaton::core::network::connection::error e);
  };
  node();
  ~node();
  uint32_t height;
  uint32_t peer_ids;  // count
  handler* handler_;
  lis_handler* lis_handler_;
  std::vector<char*> buffers;
  std::mutex buffer_mutex;
  std::map<uint32_t, automaton::core::network::acceptor*> acceptors;
  std::map<uint32_t, automaton::core::network::connection*> peers;
  char* add_buffer(uint32_t size);
  /// This function is created because the acceptor needs ids for the connections it accepts
  uint32_t get_next_peer_id();
  bool accept_connection();
  bool add_peer(uint32_t id, const std::string& connection_type, const std::string& address);
  void remove_peer(uint32_t id);
  bool add_acceptor(uint32_t id, const std::string& connection_type, const std::string& address);
  void remove_acceptor(uint32_t id);
  void send_height(uint32_t connection_id = 0);
  // process
};

#endif  // AUTOMATON_EXAMPLES_NETWORK_NODE_PROTOTYPE_H_
