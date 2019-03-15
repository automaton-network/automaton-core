#include "automaton/core/testnet/testnet.h"

#include "automaton/core/io/io.h"
#include "automaton/core/node/node.h"

namespace automaton {
namespace core {
namespace testnet {

// static

static const uint32_t STARTING_PORT = 12300;

std::unordered_map<std::string, std::shared_ptr<testnet>> testnet::testnets;

bool testnet::create_testnet(const std::string& id, const std::string& smart_protocol_id, network_protocol_type ntype,
    uint32_t number_nodes, std::unordered_map<uint32_t, std::vector<uint32_t> > peer_list) {
  auto it = testnets.find(id);
  if (it != testnets.end()) {
    LOG(ERROR) << "Testnet with id " << id << " already exists!";
    return false;
  }
  auto net = std::unique_ptr<testnet>(new testnet(id, smart_protocol_id, ntype, number_nodes));
  bool initialised = net->init();
  if (!initialised) {
    LOG(ERROR) << "Testnet " << id << " initialization failed!";
    return false;
  }
  net->connect(peer_list);
  testnets[id] = std::move(net);
  return true;
}

void testnet::destroy_testnet(const std::string& id) {
  auto it = testnets.find(id);
  if (it != testnets.end()) {
    testnets.erase(it);
  }
}

std::shared_ptr<testnet> testnet::get_testnet(std::string id) {
  auto it = testnets.find(id);
  if (it != testnets.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::string> testnet::list_testnets() {
  std::vector<std::string> result;
  for (auto it = testnets.begin(); it != testnets.end(); it++) {
    result.push_back(it->first);
  }
  return result;
}

// public

testnet::~testnet() {
  for (uint32_t i = 0; i < node_ids_list.size(); ++i) {
    automaton::core::node::node::remove_node(node_ids_list[i]);
  }
}

void testnet::connect(std::unordered_map<uint32_t, std::vector<uint32_t> > peers_list) {
  std::string id = network_id + "_";
  std::string address = "";
  uint32_t port = 0;
  if (network_type == network_protocol_type::localhost) {
    address = "tcp://127.0.0.1:";
    port = STARTING_PORT;
  } else {
    address = "sim://1:20:10000:";
  }

  for (auto it = peers_list.begin(); it != peers_list.end(); it++) {
    std::string nid = id + std::to_string(it->first);
    automaton::core::node::node* n = automaton::core::node::node::get_node(nid);
    if (n == nullptr) {
      LOG(ERROR) << "No such node: " << nid;
      continue;
    }
    std::vector<uint32_t> peers = it->second;
    for (uint32_t i = 0; i < peers.size(); ++i) {
      uint32_t pid = n->add_peer(address + std::to_string(port + peers[i]));
      n->connect(pid);
    }
  }
}

std::vector<std::string> testnet::list_nodes() {
  return node_ids_list;
}

// private

testnet::testnet(const std::string& id, const std::string& smart_protocol_id, network_protocol_type ntype,
    uint32_t number_nodes): network_id(id), protocol_id(smart_protocol_id), network_type(ntype),
    number_nodes(number_nodes) {}

bool testnet::init() {
  std::string address = "";
  uint32_t port = 0;
  if (network_type == network_protocol_type::localhost) {
    address = "tcp://127.0.0.1:";
    port = STARTING_PORT;
  } else {
    address = "sim://100:10000:";
  }
  for (uint32_t i = 1; i <= number_nodes; ++i) {
    std::string node_id = network_id + "_" + std::to_string(i);
    bool res = automaton::core::node::node::launch_node(node_id, protocol_id, address + std::to_string(port + i));
    if (!res) {
      return false;
    }
    node_ids_list.push_back(node_id);
  }
  return true;
}

}  // namespace testnet
}  // namespace core
}  // namespace automaton
