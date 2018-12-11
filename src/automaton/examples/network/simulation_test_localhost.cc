#include <cstdlib>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/network/tcp_implementation.h"
#include "automaton/examples/network/node_prototype.h"
#include "automaton/core/io/io.h"

namespace acn = automaton::core::network;

std::mutex buffer_mutex;

/// Constants

static const int FIRST_ACCEPTOR_PORT = 12345;
static const uint32_t NUMBER_NODES = 100;
// These include only the peers that a node connects to, not the accepted ones
static const uint32_t NUMBER_PEERS_IN_NODE = 1;
static const uint32_t MIN_LAG = 100;
static const uint32_t MAX_LAG = 1000;
static const uint32_t MIN_CONNECTIONS = 0;
static const uint32_t MAX_CONNECTIONS = 1;
static const uint32_t MIN_BANDWIDTH = 16;
static const uint32_t MAX_BANDWIDTH = 16;
static const uint32_t LOOP_STEP = 100;
static const uint32_t BLOCK_CREATION_STEP = 1500;
static const uint32_t MAX_SIMULATION_TIME = 10000;

const char* LOCALHOST = "127.0.0.1:";

/// Global variables

/// height -> how many connections have that height
static std::map<uint32_t, uint32_t> heights_count;
static std::vector<node*> nodes(NUMBER_NODES);

/// Function that collects and prints test results
void collect_stats() {
  heights_count.clear();
  // logging("Nodes size: " + std::to_string(nodes.size()));
  for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
    heights_count[nodes[i]->height]++;
  }
  LOG(INFO) << "==== Heights ====";
  for (auto it = heights_count.begin(); it != heights_count.end(); ++it) {
    LOG(INFO) << std::to_string(it->first) << " -> " << std::to_string(it->second);
  }
  LOG(INFO) << "=================";
}

int main() {
  try {
    acn::tcp_init();
    LOG(INFO) << "Creating acceptors...";
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      nodes[i] = new node();
      nodes[i]->add_acceptor(i, "tcp", LOCALHOST + std::to_string(FIRST_ACCEPTOR_PORT + i));
    }
    LOG(INFO) << "Creating connections...";
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      for (uint32_t j = 0; j < NUMBER_PEERS_IN_NODE; ++j) {
        nodes[i]->add_peer(nodes[i]->get_next_peer_id(), "tcp", LOCALHOST +
            std::to_string(FIRST_ACCEPTOR_PORT + std::rand() % NUMBER_NODES));
      }
    }
    LOG(INFO) << "Starting simulation...";
    // ==============================================
    for (uint32_t i = 0; i < MAX_SIMULATION_TIME; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      if (i < MAX_SIMULATION_TIME && (i+LOOP_STEP) % BLOCK_CREATION_STEP == 0) {
        int n = std::rand() % NUMBER_NODES;
        ++nodes[n]->height;
        nodes[n]->send_height();
      }
      collect_stats();
      std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
    }
    // sim->print_q();
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
  return 0;
}
