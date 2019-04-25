#include <memory>
#include <unordered_map>

#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/testnet/testnet.h"
#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

using automaton::core::node::node;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

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
    result[i] = peers;
  }
  return result;
}

int main() {
  node::register_node_type("blockchain", [](const std::string& id, const std::string& proto_id)->
      std::shared_ptr<automaton::core::node::node> {
      return std::shared_ptr<automaton::core::node::node>(new blockchain_cpp_node(id, proto_id));
    });
  if (smart_protocol::load("blockchain", "automaton/examples/smartproto/blockchain/") == false) {
    std::cout << "Blockchain protocol was NOT loaded!!!" << std::endl;
  }
  std::shared_ptr<automaton::core::network::simulation> sim = automaton::core::network::simulation::get_simulator();
  sim->simulation_start(50);
  automaton::core::network::tcp_init();
  testnet::create_testnet("blockchain", "testnet", "doesntmatter", testnet::network_protocol_type::simulation, 10,
      create_connections_vector(10, 2));

  bool stop_logger = false;
  std::thread logger([&]() {
    while (!stop_logger) {
      // Dump logs once per second.
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      for (auto n : node::list_nodes()) {
        node::get_node(n)->dump_logs("logs/blockchain/" + n + ".html");
      }
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(30000));

  stop_logger = true;
  logger.join();

  automaton::core::network::tcp_release();
  sim->simulation_stop();

  // std::cout << "ALL BLOCKCHAIN TOPS:" << std::endl;
  // for (auto n : node::list_nodes()) {
  //   std::shared_ptr<blockchain_cpp_node> node = std::dynamic_pointer_cast<blockchain_cpp_node>(node::get_node(n));
  //   std::cout << n << "\n" << node->node_stats(4) << std::endl;
  // }

  testnet::destroy_testnet("testnet");
  return 0;
}
