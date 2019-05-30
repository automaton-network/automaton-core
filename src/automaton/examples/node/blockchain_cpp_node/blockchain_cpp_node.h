#ifndef AUTOMATON_EXAMPLES_NODE_BLOCKCHAIN_CPP_NODE_BLOCKCHAIN_CPP_NODE_H_
#define AUTOMATON_EXAMPLES_NODE_BLOCKCHAIN_CPP_NODE_BLOCKCHAIN_CPP_NODE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/crypto/cryptopp/SHA3_256_cryptopp.h"
#include "automaton/core/data/factory.h"
#include "automaton/core/io/io.h"
#include "automaton/core/node/node.h"

using automaton::core::crypto::cryptopp::SHA3_256_cryptopp;

struct block {
  std::string miner;
  std::string prev_hash;
  uint64_t height;
  std::string nonce;
  block(const std::string& miner = "", const std::string& prev_hash = "", uint64_t height = 0,
      const std::string& nonce = "");
  std::string to_string() const;
  std::string data() const;
  std::string block_hash() const;
};

enum block_validity {
  VALID = 1,
  INVALID = 2,
  DUPLICATE = 3,
  NO_PARENT = 4
};

inline std::string validity_to_str(block_validity bv) {
  switch (bv) {
    case VALID: return "Valid";
    case INVALID: return "Invalid";
    case DUPLICATE: return "Duplicate";
    case NO_PARENT: return "No_parent";
  }
  return "";
}

class blockchain_cpp_node : public automaton::core::node::node {
 public:
  blockchain_cpp_node(const std::string& id, const std::string& proto_id);

  ~blockchain_cpp_node();

  void init();

  void script(const std::string& command, std::promise<std::string>* result) {}

  std::string process_cmd(const std::string& cmd, const std::string& params);

  std::string node_stats(uint32_t last_blocks);

  block get_blockchain_top();

  uint32_t get_blocks_size() {
    return blocks.size();
  }

 private:
  void s_on_blob_received(uint32_t id, const std::string& blob);
  void s_on_msg_sent(uint32_t c, uint32_t id, const automaton::core::common::status& s);
  void s_on_connected(uint32_t id);
  void s_on_disconnected(uint32_t id);
  void s_on_error(uint32_t id, const std::string& message);
  void s_update(uint64_t time);
  std::string s_debug_html();

  // Connections
  void on_block(uint32_t p_id, const block& b);
  void on_hello(uint32_t p_id, const std::string& name);
  void gossip(uint32_t peer_from, uint32_t starting_block);
  std::string get_peer_name(uint32_t id) const;

  block_validity validate_block(const block& b);
  block get_block(const std::string& hash) const;
  std::string get_current_hash() const;

  void send_block(uint32_t p_id, const std::string& hash);
  void send_blocks(uint32_t p_id, uint32_t starting_block);

  // Miner
  std::string get_target() const;
  void increment_nonce();
  block mine();

  // Logging and visualization
  void log_block(std::string identifer, block b, std::string info);
  std::unordered_map<std::string, uint32_t> collect_balances();

  //
  std::unordered_map<std::string, block> blocks;
  std::vector<std::string> blockchain;
  unsigned char nonce[16];

  std::unordered_map<uint32_t, std::string> peer_names;

  std::shared_ptr<automaton::core::data::factory> factory;
  uint32_t hello_msg_id;
  uint32_t block_msg_id;
};

#endif  // AUTOMATON_EXAMPLES_NODE_BLOCKCHAIN_CPP_NODE_BLOCKCHAIN_CPP_NODE_H_
