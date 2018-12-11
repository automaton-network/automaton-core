#include <cstdlib>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/network/simulated_connection.h"
#include "automaton/examples/network/node_prototype.h"
#include "automaton/core/io/io.h"

using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::core::network::simulation;

std::mutex buffer_mutex;
std::vector<char*> buffers;

/// Constants

static const uint32_t NUMBER_NODES = 1000;
// These include only the peers that a node connects to, not the accepted ones
static const uint32_t NUMBER_PEERS_IN_NODE = 4;
static const uint32_t MIN_LAG = 100;
static const uint32_t MAX_LAG = 1000;
static const uint32_t MIN_CONNECTIONS = 0;
static const uint32_t MAX_CONNECTIONS = 1;
static const uint32_t MIN_BANDWIDTH = 16;
static const uint32_t MAX_BANDWIDTH = 16;
static const uint32_t LOOP_STEP = 100;
static const uint32_t BLOCK_CREATION_STEP = 1500;
static const uint32_t MAX_SIMULATION_TIME = 10000;

/// Global variables

/// height -> how many connections have that height
static std::map<uint32_t, uint32_t> heights_count;
static std::vector<node*> nodes(NUMBER_NODES);

/// Helper functions for creating addresses

std::string create_connection_address(uint32_t num_acceptors, uint32_t this_acceptor,
                                      uint32_t min_lag, uint32_t max_lag,
                                      uint32_t min_bandwidth, uint32_t max_bandwidth) {
  /// Choosing random min (mn) and max lag (mx): min_lag <= mn < mx <= max_lag
  std::stringstream s;
  uint32_t mn, mx, acc;
  if (min_lag == max_lag) {
    s << min_lag << ':' << max_lag << ':';
  } else {
    mn = std::rand() % (max_lag - min_lag + 1) + min_lag;
    mx = std::rand() % (max_lag - min_lag + 1) + min_lag;
    s << (mn < mx ? mn : mx) << ':' << (mn < mx ? mx : mn) << ':';
  }
  /// For test purposes if we have n acceptors, their addresses are in range 1-n
  do {
    acc = (std::rand() % num_acceptors + 1);
  } while (acc == this_acceptor);
  s << (std::rand() % (max_bandwidth - min_bandwidth + 1) + min_bandwidth) << ':' << acc;
  // logging("Created connection address: " + s.str());
  return s.str();
}
std::string create_connection_address(uint32_t num_acceptors, uint32_t this_acceptor) {
  return create_connection_address(num_acceptors, this_acceptor, MIN_LAG, MAX_LAG,
                                  MIN_BANDWIDTH, MAX_BANDWIDTH);
}
std::string create_acceptor_address(uint32_t address,
                                    uint32_t min_connections, uint32_t max_connections,
                                    uint32_t min_bandwidth, uint32_t max_bandwidth) {
  std::stringstream s;
  uint32_t conns;
  conns = std::rand() % max_connections;
  conns = conns > min_connections ? conns : min_connections;
  s << conns << ':' << (std::rand() % (max_bandwidth - min_bandwidth + 1) + min_bandwidth) << ':'
      << address;
  // logging("Created acceptor address: " + s.str());
  return s.str();
}
std::string create_acceptor_address(uint32_t address) {
  return create_acceptor_address(address, MIN_CONNECTIONS, MAX_CONNECTIONS, MIN_BANDWIDTH,
                                MAX_BANDWIDTH);
}
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
    simulation* sim = simulation::get_simulator();
    LOG(INFO) << "Creating acceptors...";
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      nodes[i] = new node();
      nodes[i]->add_acceptor(i, "sim", create_acceptor_address(i+1));
    }
    LOG(INFO) << "Creating connections...";
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      for (uint32_t j = 0; j < NUMBER_PEERS_IN_NODE; ++j) {
        nodes[i]->add_peer(nodes[i]->get_next_peer_id(), "sim",
            create_connection_address(NUMBER_NODES, i));
      }
    }
    LOG(INFO) << "Starting simulation...";
    // ==============================================
    for (uint32_t i = 0; i < MAX_SIMULATION_TIME; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      // sim->print_connections();
      int events_processed = sim->process(i);
      LOG(INFO) << "Events processed: " << events_processed;
      if (i < MAX_SIMULATION_TIME / 2 && (i+LOOP_STEP) % BLOCK_CREATION_STEP == 0) {
        int n = std::rand() % NUMBER_NODES;
        ++nodes[n]->height;
        nodes[n]->send_height();
      }
      collect_stats();
    }
    // sim->print_q();
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
  return 0;
}
