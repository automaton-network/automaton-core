#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/examples/network/extended_node/extended_node_prototype.h"

using automaton::core::crypto::hash_transformation;
using automaton::core::crypto::SHA256_cryptopp;
using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::examples::node;

// Constants

static const int FIRST_ACCEPTOR_PORT = 11100;
static const int LAST_ACCEPTOR_PORT  = 11200;
static const uint32_t NUMBER_NODES   = 24;
// These include only the peers that a node connects to, not the accepted ones
static const uint32_t NUMBER_PEERS_IN_NODE = 2;
static const uint32_t MIN_CONNECTIONS = 0;
static const uint32_t MAX_CONNECTIONS = 1;
static const uint32_t LOOP_STEP = 48;
static const uint32_t SIMULATION_TIME = 10000;
static const uint32_t MINER_PRECISION_BITS = 20;
static const uint32_t NEW_NODES = 48;

static const char* LOCALHOST = "127.0.0.1:";

// Global variables

// height -> how many connections have that height
static std::map<std::string, uint32_t> hashes;
static std::map<std::string, uint32_t> heights;
static std::vector<node*> nodes;
std::mutex nodes_mutex;
bool simulation_end = false;
std::thread miner;
std::thread updater;

std::string create_connection_address(node* n, const node::node_params& params) {
  std::string address;
  do {
    address = LOCALHOST + std::to_string(std::rand() % (LAST_ACCEPTOR_PORT - FIRST_ACCEPTOR_PORT) +
        FIRST_ACCEPTOR_PORT);
  } while (address == n->id);
  return address;
}

// Function that collects and prints test results
void collect_stats() {
  hashes.clear();
  heights.clear();
  nodes_mutex.lock();
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    auto res = nodes[i]->get_height_and_top();
    std::string hash = automaton::core::io::string_to_hex(res.second);
    hashes[hash]++;
    heights[hash] = res.first;
    // nodes[i]->print_node_info();
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
        n->update();
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
    automaton::core::network::tcp_init();
    node::node_params params;
    params.connection_type = "tcp";
    params.connected_peers_count = NUMBER_PEERS_IN_NODE;

    LOG(INFO) << "Creating nodes...";

    try {
      for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
        nodes.push_back(new node(params, create_connection_address));
        nodes[i]->init();
        std::string address = LOCALHOST + std::to_string(FIRST_ACCEPTOR_PORT + i);
        nodes[i]->id = address;
        nodes[i]->add_acceptor("tcp", address);
      }
    } catch (std::exception& e) {
      LOG(ERROR) << "EXCEPTION " << std::string(e.what());
    } catch(...) {
      LOG(ERROR) << "UNKOWN EXCEPTION!";
    }

    LOG(INFO) << "Starting simulation...";

    updater = std::thread(update_thread_function);
    miner = std::thread(miner_thread_function);

    for (uint32_t i = 0; i < SIMULATION_TIME; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      collect_stats();
      std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
    }

    LOG(INFO) << "Adding more nodes...";

    nodes_mutex.lock();
    try {
      for (uint32_t i = NUMBER_NODES; i < NUMBER_NODES + NEW_NODES; ++i) {
        nodes.push_back(new node(params, create_connection_address));
        nodes[i]->init();
        std::string address = LOCALHOST + std::to_string(FIRST_ACCEPTOR_PORT + i);
        nodes[i]->id = address;
        nodes[i]->add_acceptor("tcp", address);
      }
    } catch (std::exception& e) {
      LOG(ERROR) << "EXCEPTION " << std::string(e.what());
    } catch(...) {
      LOG(ERROR) << "UNKOWN EXCEPTION!";
    }
    nodes_mutex.unlock();

    LOG(INFO) << "Continuing simulation...";

    for (uint32_t i = SIMULATION_TIME / 3; i < SIMULATION_TIME; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      collect_stats();
      std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " << std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }

  LOG(INFO) << "SIMULATION END!";

  simulation_end = true;
  miner.join();
  updater.join();
  collect_stats();
  automaton::core::network::tcp_release();
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    delete nodes[i];
  }
  return 0;
}
