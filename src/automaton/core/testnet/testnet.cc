#include "automaton/core/testnet/testnet.h"

#include <set>

#include "automaton/core/io/io.h"
#include "automaton/core/node/node.h"

namespace automaton {
namespace core {
namespace testnet {

// static

static const uint32_t STARTING_PORT = 12300;

std::unordered_map<std::string, std::shared_ptr<testnet>> testnet::testnets;

bool testnet::create_testnet(const std::string& node_type, const std::string& id, const std::string& smart_protocol_id,
    network_protocol_type ntype, uint32_t number_nodes, std::unordered_map<uint32_t,
    std::vector<uint32_t> > peer_list) {
  auto it = testnets.find(id);
  if (it != testnets.end()) {
    LOG(WARNING) << "Testnet with id " << id << " already exists!";
    return false;
  }
  auto net = std::unique_ptr<testnet>(new testnet(node_type, id, smart_protocol_id, ntype, number_nodes));
  bool initialised = net->init();
  if (!initialised) {
    LOG(WARNING) << "Testnet " << id << " initialization failed!";
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

std::shared_ptr<testnet> testnet::get_testnet(const std::string& id) {
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

void testnet::connect(const std::unordered_map<uint32_t, std::vector<uint32_t> >& peers_list) const {
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
    std::stringstream nid;
    nid << id << it->first;
    std::shared_ptr<automaton::core::node::node> n = automaton::core::node::node::get_node(nid.str());
    if (n == nullptr) {
      LOG(WARNING) << "No such node: " << nid.str();
      continue;
    }
    const std::vector<uint32_t>& peers = it->second;
    for (uint32_t i = 0; i < peers.size(); ++i) {
      std::stringstream ss;
      ss << address << (port + peers[i]);
      uint32_t pid = n->add_peer(ss.str());
      n->connect(pid);
    }
  }
}

std::vector<std::string> testnet::list_nodes() {
  return node_ids_list;
}

// private

testnet::testnet(const std::string& node_type, const std::string& id, const std::string& smart_protocol_id,
    network_protocol_type ntype, uint32_t number_nodes): node_type(node_type), network_id(id),
    protocol_id(smart_protocol_id), network_type(ntype), number_nodes(number_nodes), node_ids_list(number_nodes, "") {}

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
    bool res = automaton::core::node::node::launch_node(node_type,
        node_id, protocol_id, address + std::to_string(port + i));
    if (!res) {
      return false;
    }
    node_ids_list[i-1] = node_id;
  }
  return true;
}

// Helprer functions

/*
returns connection graph
n -> number of nodes
p -> number of peers
* connects node i with the next p nodes
*/
std::unordered_map<uint32_t, std::vector<uint32_t> > create_connections_vector(uint32_t n, uint32_t p) {
  std::unordered_map<uint32_t, std::vector<uint32_t> > result;
  if (p >= ((n + 1) / 2)) {
    LOG(WARNING) << "'p' is too big! Setting 'p' to max valid number of peers for 'n' = " << n << " : " <<
        ((n + 1) / 2 - 1);
    return result;
  }
  for (uint32_t i = 1; i <= n; ++i) {
    std::vector<uint32_t> peers;
    for (uint32_t j = 0; j < p; ++j) {
      peers.push_back((i + j) % n + 1);
    }
    result[i] = std::move(peers);
  }
  return result;
}

std::unordered_map<uint32_t, std::vector<uint32_t> > create_rnd_connections_vector(uint32_t n, uint32_t p) {
  std::unordered_map<uint32_t, std::vector<uint32_t> > result;
  uint32_t k;
  if (p >= ((n + 1) / 2)) {
    LOG(WARNING) << "'p' is too big! Setting 'p' to max valid number of peers for 'n' = " << n << " : " <<
        ((n + 1) / 2 - 1);
    return result;
  }
  for (uint32_t i = 1; i <= n; ++i) {
    std::set<uint32_t> peers;
    while (peers.size() < p) {
      k = std::rand() % n + 1;
      if (k == i) {continue;}
      peers.insert(k);
    }
    result[i] = std::vector<uint32_t>(peers.begin(), peers.end());
  }
  return result;
}

}  // namespace testnet
}  // namespace core
}  // namespace automaton
