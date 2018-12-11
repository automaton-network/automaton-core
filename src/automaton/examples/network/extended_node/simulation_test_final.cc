#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/examples/network/extended_node/extended_node_prototype.h"

using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::core::network::simulation;
using automaton::examples::node;

/// Constants

static const uint32_t NUMBER_NODES = 1000;
static const uint32_t NEW_NODES = 25;
// These include only the peers that a node connects to, not the accepted ones
static const uint32_t NUMBER_PEERS_IN_NODE = 4;
static const uint32_t MIN_LAG = 10;
static const uint32_t MAX_LAG = 100;
static const uint32_t MIN_CONNECTIONS = 0;
static const uint32_t MAX_CONNECTIONS = 1;
static const uint32_t MIN_BANDWIDTH = 512;
static const uint32_t MAX_BANDWIDTH = 512;
static const uint32_t LOOP_STEP = 100;
static const uint32_t SIMULATION_TIME = 10000;

const uint32_t MINER_PRECISION_BITS = 17;

/// Global variables

/// height -> how many connections have that height
static std::map<std::string, uint32_t> hashes;
static std::map<std::string, uint32_t> heights;
static std::vector<node*> nodes;
std::mutex nodes_mutex;
bool simulation_end = false;
std::thread miner;
std::thread updater;

/// Helper functions for creating addresses
std::string create_connection_address(node* n, const node::node_params& params) {
  std::stringstream s;
  uint32_t mn, mx, acc;
  if (MIN_LAG == MAX_LAG) {
    s << MIN_LAG << ':' << MAX_LAG << ':';
  } else {
    mn = std::rand() % (MAX_LAG - MIN_LAG + 1) + MIN_LAG;
    mx = std::rand() % (MAX_LAG - MIN_LAG + 1) + MIN_LAG;
    s << (mn < mx ? mn : mx) << ':' << (mn < mx ? mx : mn) << ':';
  }
  /// For test purposes if we have n acceptors, their addresses are in range 1-n
  uint32_t id = std::stoul(n->id);
  do {
    acc = (std::rand() % NUMBER_NODES + 1);
  } while (acc == id);
  s << params.bandwidth << ':' << acc;
  return s.str();
}

std::string create_acceptor_address(uint32_t address, const node::node_params& params) {
  std::stringstream s;
  s << "1:" << params.bandwidth << ':' << address;
  return s.str();
}

void collect_stats() {
  hashes.clear();
  heights.clear();
  nodes_mutex.lock();
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    auto res = nodes[i]->get_height_and_top();
    std::string hash = automaton::core::io::string_to_hex(res.second);
    hashes[hash]++;
    heights[hash] = res.first;
  }
  LOG(INFO) << "==== Heights ====";
  for (auto it = hashes.begin(); it != hashes.end(); ++it) {
    LOG(INFO) << "HASH: " << it->first << " AT HEIGHT: " << heights[it->first] << " #PEERS: "
        << std::to_string(it->second);
  }
  LOG(INFO) << "=================";
  nodes_mutex.unlock();
}

void miner_thread_function() {
  try {
    while (!simulation_end) {
      nodes_mutex.lock();
      uint32_t n_number = nodes.size();
      nodes_mutex.unlock();
      for (uint32_t i = 0; i < n_number && !simulation_end; ++i) {
        nodes_mutex.lock();
        node* n = nodes[i];
        nodes_mutex.unlock();
        n->mine(128, MINER_PRECISION_BITS);
      }
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
}

void update_thread_function() {
  try {
    while (!simulation_end) {
      nodes_mutex.lock();
      uint32_t n_number = nodes.size();
      nodes_mutex.unlock();
      for (uint32_t i = 0; i < n_number && !simulation_end; ++i) {
        nodes_mutex.lock();
        node* n = nodes[i];
        nodes_mutex.unlock();
        n->update();
      }
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
}

int main() {
  try {
    simulation* sim = simulation::get_simulator();
    LOG(INFO) << "Creating acceptors...";
    node::node_params params;
    params.connection_type = "sim";
    params.connected_peers_count = NUMBER_PEERS_IN_NODE;
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      params.bandwidth = (std::rand() % (MAX_BANDWIDTH - MIN_BANDWIDTH + 1) + MIN_BANDWIDTH);
      nodes.push_back(new node(params, create_connection_address));
      nodes[i]->id = std::to_string(i);
      nodes[i]->init();
      nodes[i]->add_acceptor("sim", create_acceptor_address(i+1, params));
    }
    LOG(INFO) << "Starting simulation...";
    updater = std::thread(update_thread_function);
    miner = std::thread(miner_thread_function);
    // ==============================================
    for (uint32_t i = 0; i < 2 * SIMULATION_TIME / 3; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      int32_t events_processed = sim->process(i);
      LOG(INFO) << "Events processed: " << events_processed;
      collect_stats();
      std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
    }
    nodes_mutex.lock();
    for (uint32_t i = NUMBER_NODES; i < NUMBER_NODES + NEW_NODES; ++i) {
      params.bandwidth = (std::rand() % (MAX_BANDWIDTH - MIN_BANDWIDTH + 1) + MIN_BANDWIDTH);
      nodes.push_back(new node(params, create_connection_address));
      nodes[i]->id = std::to_string(i);
      nodes[i]->init();
      nodes[i]->add_acceptor("sim", create_acceptor_address(i+1, params));
    }
    nodes_mutex.unlock();
    LOG(INFO) << "Continuing simulation...";
    for (uint32_t i =  2 * SIMULATION_TIME / 3; i < SIMULATION_TIME * 2; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      int32_t events_processed = sim->process(i);
      LOG(INFO) << "Events processed: " << events_processed;
      collect_stats();
      std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
  simulation_end = true;
  miner.join();
  updater.join();
  collect_stats();
  for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
    delete nodes[i];
  }
  return 0;
}
