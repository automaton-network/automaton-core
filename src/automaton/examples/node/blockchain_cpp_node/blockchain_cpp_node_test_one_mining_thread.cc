#include <chrono>
#include <memory>
#include <unordered_map>

#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/testnet/testnet.h"
#include "automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.h"

using std::chrono::system_clock;

using automaton::core::io::bin2hex;
using automaton::core::node::node;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

auto WORKER_SLEEP_TIME_MS = std::chrono::milliseconds(20);

bool worker_running = true;

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
  testnet::create_testnet("blockchain", "testnet", "doesntmatter", testnet::network_protocol_type::simulation, 1000,
      create_connections_vector(1000, 4));

  std::unordered_map<std::string, std::mutex> node_locks;
  std::mutex map_lock;
  std::thread worker_thread = std::thread([&]() {
      while (worker_running) {
        std::vector<std::string> node_ids = node::list_nodes();
        std::string id = node_ids[std::rand()%1000];
        map_lock.lock();
        bool locked = node_locks[id].try_lock();
        map_lock.unlock();
        if (locked) {
          auto node = node::get_node(id);
          if (node != nullptr) {
            // protocol update time and node's time to update are not used here
            node->process_update(0);
          }
          node_locks[id].unlock();
        } else {
          continue;
        }
        std::this_thread::sleep_for(WORKER_SLEEP_TIME_MS);
      }
    });

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

  std::this_thread::sleep_for(std::chrono::milliseconds(180000));

  worker_running = false;
  worker_thread.join();

  stop_logger = true;
  logger.join();

  automaton::core::network::tcp_release();
  sim->simulation_stop();

  std::unordered_map<std::string, uint32_t> counter;
  std::unordered_map<std::string, block> blocks;

  std::cout << "ALL BLOCKCHAIN TOPS:" << std::endl;
  for (auto n : node::list_nodes()) {
    std::shared_ptr<blockchain_cpp_node> node = std::dynamic_pointer_cast<blockchain_cpp_node>(node::get_node(n));
    block b = node->get_blockchain_top();
    counter[b.block_hash()]++;
    blocks[b.block_hash()] = b;
  }
  std::cout << "=============================================" << std::endl;
  for (auto it = counter.begin(); it != counter.end(); ++it) {
    std::cout << bin2hex(it->first) << "@ height " << blocks[it->first].height << " -> " << it->second <<
        " nodes" << std::endl;
  }

  testnet::destroy_testnet("testnet");
  return 0;
}
