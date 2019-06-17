#include <memory>
#include <set>
#include <unordered_map>

#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/node/node_updater.h"
#include "automaton/core/testnet/testnet.h"
#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

using automaton::core::io::bin2hex;
using automaton::core::node::node;
using automaton::core::node::node_updater_tests;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

const uint32_t WORKER_SLEEP_TIME_MS = 1000;
const uint32_t NODES = 100000;
const uint32_t PEERS = 2;
const uint32_t SIMULATION_SLEEP_TIME_MS = 30;
const uint32_t LOGGER_SLEEP_TIME_MS = 100;
const uint32_t SIMULATION_TIME = 120000;  // from updater.start() to .stop(), doesn't include time to connect

/*
returns connection graph
n -> number of nodes
p -> number of peers
* connects node i with the next p nodes
*/
std::unordered_map<uint32_t, std::vector<uint32_t> > create_connections_vector(uint32_t n, uint32_t p) {
  std::unordered_map<uint32_t, std::vector<uint32_t> > result;
  if (p >= ((n + 1) / 2)) {
    std::cout << "'p' is too big! Setting 'p' to max valid number of peers for 'n' = " << n << " : " <<
        ((n + 1) / 2 - 1) << std::endl;
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
    std::cout << "'p' is too big! Setting 'p' to max valid number of peers for 'n' = " << n << " : " <<
        ((n + 1) / 2 - 1) << std::endl;
    return result;
  }
  for (uint32_t i = 1; i <= n; ++i) {
    std::set<uint32_t> peers;
    while (peers.size() < p) {
      k = std::rand() % NODES + 1;
      if (k == i) {continue;}
      peers.insert(k);
    }
    result[i] = std::vector<uint32_t>(peers.begin(), peers.end());
  }
  return result;
}

int main() {
  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  node::register_node_type("blockchain", [](const std::string& id, const std::string& proto_id)->
      std::shared_ptr<automaton::core::node::node> {
      return std::shared_ptr<automaton::core::node::node>(new blockchain_cpp_node(id, proto_id));
    });
  if (smart_protocol::load("blockchain", "automaton/examples/smartproto/blockchain/") == false) {
    std::cout << "Blockchain protocol was NOT loaded!!!" << std::endl;
  }

  std::shared_ptr<automaton::core::network::simulation> sim = automaton::core::network::simulation::get_simulator();
  sim->simulation_start(SIMULATION_SLEEP_TIME_MS);

  testnet::create_testnet("blockchain", "testnet", "doesntmatter", testnet::network_protocol_type::simulation, NODES,
      create_rnd_connections_vector(NODES, PEERS));

  std::vector<std::string> ids = testnet::get_testnet("testnet")->list_nodes();
  node_updater_tests updater(WORKER_SLEEP_TIME_MS, std::set<std::string>(ids.begin(), ids.end()));
  updater.start();

  bool stop_logger = true;
  std::thread logger([&]() {
    while (!stop_logger) {
      std::this_thread::sleep_for(std::chrono::milliseconds(LOGGER_SLEEP_TIME_MS));
      for (auto n : node::list_nodes()) {
        node::get_node(n)->dump_logs("logs/blockchain/" + n + ".html");
      }
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATION_TIME));

  updater.stop();
  stop_logger = true;
  logger.join();

  sim->simulation_stop();

  std::unordered_map<std::string, uint32_t> counter;
  std::unordered_map<std::string, block> blocks;

  std::unordered_map<uint32_t, uint32_t> blocks_sz;

  std::cout << "ALL BLOCKCHAIN TOPS:" << std::endl;
  for (auto n : node::list_nodes()) {
    std::shared_ptr<blockchain_cpp_node> node = std::dynamic_pointer_cast<blockchain_cpp_node>(node::get_node(n));
    block b = node->get_blockchain_top();
    counter[b.block_hash()]++;
    blocks[b.block_hash()] = b;
    blocks_sz[node->get_blocks_size()]++;
    if (blocks_sz[node->get_blocks_size()] == 1) {
      std::cout << "check " << n << std::endl;
      node::get_node(n)->dump_logs("logs/blockchain/" + n + ".html");
    }
  }
  std::cout << "=============================================" << std::endl;
  for (auto it = counter.begin(); it != counter.end(); ++it) {
    std::cout << bin2hex(it->first) << " @ height " << blocks[it->first].height << " -> " << it->second <<
        " nodes" << std::endl;
  }

  for (auto it = blocks_sz.begin(); it != blocks_sz.end(); ++it) {
    std::cout << it->first << " -> " << it->second << " nodes" << std::endl;
  }
  testnet::destroy_testnet("testnet");
  return 0;
}
