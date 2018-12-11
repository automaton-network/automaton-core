#ifndef AUTOMATON_EXAMPLES_NETWORK_EXTENDED_NODE_EXTENDED_NODE_PROTOTYPE_H_
#define AUTOMATON_EXAMPLES_NETWORK_EXTENDED_NODE_EXTENDED_NODE_PROTOTYPE_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/data/schema.h"
#include "automaton/core/data/factory.h"
#include "automaton/core/data/msg.h"
#include "automaton/core/data/protobuf/protobuf_msg.h"
#include "automaton/core/state/state.h"
#include "automaton/core/network/connection.h"
#include "automaton/core/network/acceptor.h"

namespace automaton {
namespace examples {

// Class node
class node: public core::network::connection::connection_handler,
    public core::network::acceptor::acceptor_handler {
 public:
  // TODO(kari): This need to be deleted.
  static core::data::factory* msg_factory;
  struct block {
    std::string hash;
    std::string prev_hash;
    uint32_t height;
    std::string miner;
    std::string nonce;

    std::string to_string() const;

    block();

    block(std::string hash, std::string prev_hash, uint32_t height, std::string miner,
        std::string nonce);
  };

  // This should be msg
  struct node_params {
    std::string connection_type;  // "tcp" or "sim"
    uint32_t connected_peers_count;
    uint32_t bandwidth;  // simulated connection
    uint32_t timeout;

    node_params();
  };

  struct peer_data {
    std::string id;
    std::chrono::time_point<std::chrono::system_clock> last_used;
    // char* buffer;
    // other data

    peer_data();
  };

  node(node_params params,
      std::string (*get_random_acceptor_address)(node* n, const node_params& params),
      std::string (*get_random_peer_address)(node* n, const node_params& params));

  ~node();

  bool init();

  std::string (*get_random_acceptor_address)(node* n, const node_params& params);

  std::string (*get_random_peer_address)(node* n, const node_params& params);

  std::vector<std::string> logger;

  void add_to_log(const std::string& e);

  void log_to_stream(std::ostream& os) const;

  // This function is created because the acceptor needs ids for the connections it accepts
  uint32_t get_next_peer_id();

  bool accept_connection();

  bool add_peer(const std::string& connection_type, const std::string& address);

  bool add_peer(automaton::core::network::connection* c, const std::string& address);

  void remove_peer(const std::string& address);

  automaton::core::network::connection* get_peer(const std::string& address);

  bool add_acceptor(const std::string& connection_type, const std::string& address);

  automaton::core::network::acceptor* get_acceptor(const std::string& address);

  void remove_acceptor(const std::string& id);

  std::string get_peer_id(automaton::core::network::connection* c);

  void send_message(const std::string& message, automaton::core::network::connection* = nullptr);

  void handle_block(const std::string& hash, const block& block_,
      const std::string& serialized_block);

  std::string get_id() const;

  void set_id(const std::string& new_id);

  std::pair<uint32_t, std::string> get_height_and_top() const;

  std::string get_top() const;

  uint32_t get_height() const;

  void process(core::data::msg* input_message, automaton::core::network::connection* c = nullptr);

  /**
    Mine and update should be called from new threads. For now are called from a thread in the
    simulation until every node starts its own threads.
  */

  void mine(uint32_t number_tries, uint32_t required_leading_zeros);

  void update();

  std::string node_info() const;

 private:
  std::string id;
  node_params params;
  std::string first_block_hash;
  std::string chain_top;
  uint32_t height;
  bool initialized;
  // TODO(kari): remove this and add hello message passing id as well as a list of ids and peers
  uint32_t peer_ids;  // count
  std::vector<char*> buffers;
  std::mutex buffer_mutex;
  std::mutex global_state_mutex;
  std::mutex orphan_blocks_mutex;
  std::mutex peer_ids_mutex;
  mutable std::mutex id_mutex;
  mutable std::mutex peers_mutex;
  mutable std::mutex acceptors_mutex;
  mutable std::mutex chain_top_mutex;
  mutable std::mutex height_mutex;
  mutable std::mutex log_mutex;
  std::map<std::string, block> orphan_blocks;
  std::map<std::string, core::network::acceptor*> acceptors;
  // std::map<std::string, core::network::connection*> peers;
  std::map<core::network::connection*, peer_data> peers;
  core::crypto::hash_transformation* hasher;
  core::state::state* global_state;  // map block_hash -> serialized msg, containing the block

  // Inherited handlers' functions

  void on_message_received(core::network::connection* c, char* buffer,
      uint32_t bytes_read, uint32_t id);

  void on_message_sent(core::network::connection* c, uint32_t id,
      core::network::connection::error e);

  void on_connected(core::network::connection* c);

  void on_disconnected(core::network::connection* c);

  void on_error(core::network::connection* c, core::network::connection::error e);

  bool on_requested(core::network::acceptor* a, const std::string& address);

  void on_connected(core::network::acceptor* a, core::network::connection* c,
      const std::string& address);

  void on_error(core::network::acceptor* a, core::network::connection::error e);

  void check_orphans(const std::string& hash);

  std::string create_send_blocks_message(std::vector<std::string> hashes);

  std::string create_request_blocks_message(std::vector<std::string> hashes);

  // Helper functions

  char* add_buffer(uint32_t size);

  block msg_to_block(core::data::msg* message) const;

  std::unique_ptr<core::data::msg> block_to_msg(const block& block) const;

  std::string hash_block(const block& block) const;

  std::string add_header(const std::string& message) const;

  // Miner

  uint8_t* nonce;

  void increase_nonce();

  bool is_hash_valid(uint8_t* hash, uint8_t required_leading_zeros);
};

}  // namespace examples
}  // namespace automaton

#endif  // AUTOMATON_EXAMPLES_NETWORK_EXTENDED_NODE_EXTENDED_NODE_PROTOTYPE_H_
