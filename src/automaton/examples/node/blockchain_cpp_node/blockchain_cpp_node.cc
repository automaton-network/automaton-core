#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

#include <memory>
#include <sstream>

#include "automaton/core/data/factory.h"
#include "automaton/core/io/io.h"
#include "automaton/core/smartproto/smart_protocol.h"

using automaton::core::data::factory;
using automaton::core::data::msg;
using automaton::core::io::bin2hex;
using automaton::core::io::hex2bin;

// Helper functions

inline static std::string hash(const std::string& data) {
  SHA3_256_cryptopp sha3;
  uint8_t digest[32];
  sha3.calculate_digest(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  return std::string(reinterpret_cast<char*>(digest), 32);
}

inline static std::string hashstr(const std::string& hash) {
  std::string hex = automaton::core::io::bin2hex(hash);
  return hex.substr(hex.size() - 8);
}

static const bool LOG_ENABLED = false;

static const std::string GENESIS_HASH = hash("automaton");  // NOLINT

static const uint32_t DIFFICULTY_LEADING_ZEROS = 1;
static const char* DIFFICULTY_PREFIX = "00FFFF";

static const uint32_t MINE_ATTEMPTS = 7;

block::block(const std::string& miner = "", const std::string& prev_hash = "", uint64_t height = 0,
    const std::string& nonce = ""):miner(miner), prev_hash(prev_hash), height(height), nonce(nonce) {}

std::string block::to_string() const {
  std::stringstream ss;
  ss << "miner: " << miner << " prev_hash(hex): " << bin2hex(prev_hash) << " height: " << height << " nonce(hex): " <<
      bin2hex(nonce);
  return ss.str();
}

std::string block::data() const {
  std::stringstream ss;
  ss << miner << prev_hash << height << nonce;
  return ss.str();
}

blockchain_cpp_node::blockchain_cpp_node(const std::string& id, const std::string& proto_id) :
    node(id, "blockchain") {}

blockchain_cpp_node::~blockchain_cpp_node() {}

void blockchain_cpp_node::init() {
  peer_names[0] = "ME[" + nodeid + "]";
  factory = automaton::core::smartproto::smart_protocol::get_protocol(protoid)->get_factory();
  hello_msg_id = find_message_id("Hello", factory);
  block_msg_id = find_message_id("Block", factory);
  std::memset(nonce, 0, 16);
}

std::string blockchain_cpp_node::process_cmd(const std::string& cmd, const std::string& params) {
  // TODO(Kari) Implement it.
  return "";
}

void blockchain_cpp_node::s_on_blob_received(uint32_t id, const std::string& blob) {
  msg* m = get_wire_msg(blob).release();
  // TODO(kari): put a map [id->function]
  std::string msg_type = m->get_message_type();
  if (msg_type == "Hello") {
    on_hello(id, m->get_blob(1));
  } else if (msg_type == "Block") {
    block b;
    b.miner = m->get_blob(1);
    b.prev_hash = m->get_blob(2);
    b.height = m->get_uint64(3);
    b.nonce = m->get_blob(4);
    on_block(id, b);
  } else {
    LOG(INFO) << "Received message " << msg_type << " which is not supported!";
  }
  delete m;
}

void blockchain_cpp_node::s_on_msg_sent(uint32_t c, uint32_t id, const automaton::core::common::status& s) {}

void blockchain_cpp_node::s_on_connected(uint32_t p_id) {
  if (LOG_ENABLED) {
    log("connections", "CONNECTED TO " + std::to_string(p_id));
  }
  peer_names[p_id] = "N/A";
  msg* hello_msg = create_msg_by_id(hello_msg_id, factory).release();
  hello_msg->set_blob(1, nodeid);
  send_message(p_id, *hello_msg, 1);
  delete hello_msg;
}

void blockchain_cpp_node::s_on_disconnected(uint32_t id) {
  auto it = peer_names.find(id);
  if (it != peer_names.end()) {
    peer_names.erase(it);
  }
}

void blockchain_cpp_node::s_on_error(uint32_t id, const std::string& message) {
  LOG(ERROR) << "Error node: " << nodeid << "! " << message;
}

void blockchain_cpp_node::s_update(uint64_t time) {
  block b = mine();
  if (b.height) {
    if (LOG_ENABLED) {
      log_block("miner", b, "MINED");
    }
    on_block(0, b);
  }
}

std::string blockchain_cpp_node::s_debug_html() {
  std::stringstream nodes, edges;
  // GENESIS_HASH
  nodes <<
      "{id: '" << hashstr(GENESIS_HASH) << "', shape: 'box', label: 'GENESIS', color: '#D2B4DE', level: 0}";

  std::string clr;
  for (auto it : blocks) {
    std::string hash = it.first;
    block b = it.second;
    std::string short_hash = hashstr(hash);
    // check if this is in current blockchain
    if (hash == blockchain[b.height - 1]) {
      clr = "'#cce0ff', font: {face:'Play'}";
    } else {
      clr = "'#f2e6d9', font: {color:'#333', face:'Play'}";
    }
    std::stringstream ss;
    ss << ",\n{id: '" << short_hash << "', shape: 'box', label: '" << short_hash << "', color: "<< clr <<
        ", level: " << b.height << ", title: '" << "mined by " << b.miner << "<br>HASH: " << bin2hex(hash) <<
        "<br>HEIGHT: " << b.height <<"'}";
    nodes << ss.str();
    edges << "{from: '" << hashstr(b.prev_hash) << "', to: '" << short_hash << "', arrows:'to'}" << ",\n";
  }

  std::unordered_map<std::string, uint32_t> balances = collect_balances();
  std::stringstream balances_stream;
  for (auto it = balances.begin(); it != balances.end(); ++it) {
    balances_stream <<
    R"(<tr>
      <td>)" << it->first << R"(</td>
      <td>)" << it->second << R"(</td>
    </tr>)" << '\n';
  }

  std::stringstream html_stream;
  html_stream <<
R"HTML(<script type="text/javascript" charset="utf-8" src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
<link rel="stylesheet" type="text/css" href="https://cdn.datatables.net/1.10.19/css/jquery.dataTables.css">

<script type="text/javascript" charset="utf8" src="https://cdn.datatables.net/1.10.19/js/jquery.dataTables.js"></script>
<div id="mynetwork"></div>

<script type="text/javascript">
  // create an array with nodes
  var nodes = new vis.DataSet([
)HTML"
  << nodes.str() <<
R"(]);
  // create an array with edges
  var edges = new vis.DataSet([
)"
  << edges.str() <<
R"HTML(]);

  // create a network
  var container = document.getElementById('mynetwork');
  var data = {
    nodes: nodes,
    edges: edges
  };
  var options = {
    edges: {
      smooth: {
        type: 'cubicBezier',
        forceDirection: 'horizontal',
        roundness: 0.4
      }
    },
    layout: {
      hierarchical: {
        direction: "LR",
        levelSeparation: 120,
        nodeSpacing: 100
      }
    },
    physics: false
  };
  var network = new vis.Network(container, data, options);
  $(document).ready( function () {
    $('#balances').DataTable({
        "scrollY":        "400",
        "scrollCollapse": true,
        "paging":         false,
        "searching":      false,
    });
  } );
</script>
<font size="2" face="Courier New" >
<style>
table {
  text-align: center;
  border: 1px solid #c5cbd6;
}

</style>
<table id="balances" class="display compact">
  <thead>
    <tr>
      <th>Miner</th>
      <th>Number of blocks</th>
    </tr>
  </thead>
  <tbody>
)HTML"
  << balances_stream.str() <<
R"HTML(</tbody>
</table>
</font>
)HTML";

  return html_stream.str();
}

void blockchain_cpp_node::on_block(uint32_t p_id, const block& b) {
  block_validity validity = validate_block(b);
  std::string bhash = hash(b.data());
  if (LOG_ENABLED) {
    log(get_peer_name(p_id), "RECV | " + bin2hex(bhash) + " | " + validity_to_str(validity) + " | " + b.to_string());
  }
  if (validity == VALID) {
    // Block is valid, store it
    blocks[bhash] = b;
    // Check if we get a longer chain. Does not matter if it is the main or alternative.
    if (b.height == blockchain.size() + 1) {
      // We are sure that this is the head of the longest chain.
      blockchain.push_back(bhash);
      // Check if blocks[block.prev_hash] is part of the main chain and replace if necessary.
      int64_t block_height = blockchain.size() - 1;
      std::string longest_chain_hash = b.prev_hash;
      while (block_height > 0 && blockchain[block_height - 1] != longest_chain_hash) {
        blockchain[block_height - 1] = longest_chain_hash;
        longest_chain_hash = blocks[longest_chain_hash].prev_hash;
        --block_height;
      }
      gossip(p_id, block_height + 1);
    }
  }
}

void blockchain_cpp_node::on_hello(uint32_t p_id, const std::string& name) {
  std::stringstream ss;
  ss << "Hello from peer " << p_id << " name: " << name;
  if (LOG_ENABLED) {
    log("HELLO", ss.str());
  }
  peer_names[p_id] = name;
  send_blocks(p_id, 1);
}

void blockchain_cpp_node::gossip(uint32_t peer_from, uint32_t starting_block) {
  for (uint32_t i : list_connected_peers()) {
    if (i != peer_from) {
      send_blocks(i, starting_block);
    }
  }
}

std::string blockchain_cpp_node::get_peer_name(uint32_t id) const {
  auto it = peer_names.find(id);
  if (it == peer_names.end()) {
    return "Unknown";
  }
  return it->second;
}

block_validity blockchain_cpp_node::validate_block(const block& b) {
  std::string bhash = hash(b.data());
  std::string target = get_target();
  // Check if we already have the block
  if (blocks.find(bhash) != blocks.end()) {
    if (LOG_ENABLED) {
      log_block("validate", b, "DUPLICATE");
    }
    return DUPLICATE;
  } else if (bhash.compare(target) > 0) {  // Check difficulty
    if (LOG_ENABLED) {
      log_block("validate", b, "INVALID hash > target");
    }
    return INVALID;
  } else if (b.height < 1) {  // Check block height is a positive integer
    if (LOG_ENABLED) {
      log_block("validate", b, "INVALID height < 1");
    }
    return INVALID;
  } else if (b.prev_hash != GENESIS_HASH && blocks.find(b.prev_hash) == blocks.end()) {  // The block should have its
  // predecessor in blocks unless it is the first block
    if (LOG_ENABLED) {
      log_block("validate", b, "NO_PARENT");
    }
    return NO_PARENT;
  } else if ((b.height == 1 && b.prev_hash != GENESIS_HASH) ||
      (b.height > 1 && blocks[b.prev_hash].height != b.height - 1)) {
  // 1. If this is the first block, it needs to have GENESIS_HASH as prev_hash.
  // 2. If it is not the first block, check if the height of
  //    the block with hash prev_hash is the height of this
    if (LOG_ENABLED) {
      log_block("validate", b, "INVALID height");
    }
    return INVALID;
  }
  if (LOG_ENABLED) {
    log_block("validate", b, "VALID");
  }
  return VALID;
}

block blockchain_cpp_node::get_block(const std::string& hash) const {
  auto it = blocks.find(hash);
  if (it != blocks.end()) {
    return it->second;
  }
  return block();  // Empty or genesis block
}

std::string blockchain_cpp_node::get_current_hash() const {
  if (blockchain.size()) {
    return blockchain[blockchain.size() - 1];
  }
  return GENESIS_HASH;
}

void blockchain_cpp_node::send_block(uint32_t p_id, const std::string& hash) {
  if (LOG_ENABLED) {
    log(get_peer_name(p_id), "SEND | " + bin2hex(hash));
  }
  // Get block by hash && check if it's valid
  block b = get_block(hash);
  if (b.height == 0) {
    if (LOG_ENABLED) {
      log(get_peer_name(p_id), "Trying to send invalid block or genesis block with hash " + bin2hex(hash));
    }
    return;
  }
  msg* block_msg = create_msg_by_id(block_msg_id, factory).release();
  block_msg->set_blob(1, b.miner);
  block_msg->set_blob(2, b.prev_hash);
  block_msg->set_uint64(3, b.height);
  block_msg->set_blob(4, b.nonce);
  send_message(p_id, *block_msg, 1);
  delete block_msg;
}

void blockchain_cpp_node::send_blocks(uint32_t p_id, uint32_t starting_block) {
  if (LOG_ENABLED) {
    std::stringstream ss;
    ss << "Sending blocks " << starting_block << ".." << blockchain.size();
    log(get_peer_name(p_id), ss.str());
  }
  for (uint64_t i = starting_block - 1; i < blockchain.size(); ++i) {
    send_block(p_id, blockchain[i]);
  }
}

// Miner

std::string blockchain_cpp_node::get_target() const {
  std::string hex = std::string(DIFFICULTY_LEADING_ZEROS*2, '0') + DIFFICULTY_PREFIX +
      std::string((32-DIFFICULTY_LEADING_ZEROS-3)*2, '0');
  return hex2bin(hex);
}

void blockchain_cpp_node::increment_nonce() {
  for (uint32_t i = 15; i >= 0; --i) {
    if (nonce[i] < 255) {
      nonce[i]++;
      break;
    } else {
      nonce[i] = 0;
    }
  }
}

block blockchain_cpp_node::mine() {
  static std::string target_hash = get_target();
  std::string bhash;
  block b(nodeid, get_current_hash(), blockchain.size() + 1);
  for (uint32_t i = 0; i < MINE_ATTEMPTS; ++i) {
    b.nonce = std::string(reinterpret_cast<char*>(nonce), 16);
    bhash = hash(b.data());
    if (bhash.compare(target_hash) < 1) {
      return b;
    } else {
      increment_nonce();
    }
  }
  // Invalidate block to show no new blocks were found
  b.height = 0;
  return b;
}

// Logging && visualization
std::string blockchain_cpp_node::node_stats(uint32_t last_blocks = 0) {
  std::stringstream ss;
  int32_t starting_block = 0;
  if (last_blocks < blockchain.size() && last_blocks > 0) {
    starting_block = blockchain.size() - last_blocks;
  }
  for (uint32_t i = starting_block; i < blockchain.size(); ++i) {
    ss << bin2hex(blockchain[i]) << "\n";
  }
  return ss.str();
}

std::unordered_map<std::string, uint32_t> blockchain_cpp_node::collect_balances() {
  std::unordered_map<std::string, uint32_t> balances;
  for (uint64_t i = 0; i < blockchain.size(); ++i) {
    block b = get_block(blockchain[i]);
    balances[b.miner]++;
  }
  return balances;
}

void blockchain_cpp_node::log_block(std::string identifer, block b, std::string info) {
  std::stringstream ss;
  ss << bin2hex(hash(b.data())) << " | " << b.height << " | " << b.miner << " | " << info;
  log(identifer, ss.str());
}
