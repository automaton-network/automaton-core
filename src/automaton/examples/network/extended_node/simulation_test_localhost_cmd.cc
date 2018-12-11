#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/examples/network/extended_node/extended_node_prototype.h"

using automaton::core::crypto::hash_transformation;
using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::examples::node;

// Constants

// static const uint32_t NUMBER_NODES = 64;
static const uint32_t NUMBER_PEERS_IN_NODE = 8;
static const uint32_t LOOP_STEP = 50;
static const uint32_t SIMULATION_TIME = 6000;
static const uint32_t MINER_PRECISION_BITS = 20;
static const uint32_t TIMEOUT = 300;

static const char* LOCALHOST = "192.168.0.101:";  // "127.0.0.1:";
static const char* FILE_NAME = "simulation_output.txt";

// Global variables

static bool IS_LOCALHOST;
static char* MY_IP;
static uint32_t MIN_PORT = 12000;
static uint32_t MAX_PORT = 12030;
static uint32_t MY_MIN_PORT = 0;
static uint32_t MY_MAX_PORT = 0;
static uint32_t NUMBER_NODES = 0;
static std::vector<std::string> KNOWN_IPS;
static std::ofstream output_file(FILE_NAME, std::fstream::out);

// height->how many connections have that height
static std::map<std::string, uint32_t> hashes;
static std::map<std::string, uint32_t> heights;
static std::vector<node*> nodes;
std::mutex nodes_mutex;
bool simulation_end = false;
std::thread miner;
std::thread updater;

std::string create_localhost_connection_address(node* n, const node::node_params& params) {
  std::string address;
  uint32_t port;
  do {
    port = std::rand() % (MAX_PORT - MIN_PORT + 1) + MIN_PORT;
  } while (port >= MY_MIN_PORT && port <= MY_MAX_PORT);
  address = LOCALHOST + std::to_string(port);
  return address;
}

std::string create_remote_connection_address(node* n, const node::node_params& params) {
  return KNOWN_IPS[std::rand() % KNOWN_IPS.size()] +
      std::to_string(std::rand() % (MAX_PORT - MIN_PORT + 1) + MIN_PORT);
}

std::string create_localhost_acceptor_address(node* n, const node::node_params& params) {
  return LOCALHOST + std::to_string(MY_MIN_PORT +
      std::rand() % (MY_MAX_PORT - MY_MIN_PORT + 1));
}

std::string create_remote_acceptor_address(node* n, const node::node_params& params) {
  return MY_IP + std::to_string(MIN_PORT + std::rand() % (MAX_PORT - MIN_PORT + 1));
}

// Function that collects and prints test results
void collect_stats() {
  // LOG(INFO) << "STATS: 0";
  hashes.clear();
  heights.clear();
  nodes_mutex.lock();
  CHECK(nodes.size() == NUMBER_NODES) << "Number of nodes is not as expected! Have " <<
      nodes.size() << " expected " << NUMBER_NODES;
  for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
    // LOG(INFO) << "STATS loop: " << i;
    auto res = nodes[i]->get_height_and_top();
    std::string hash = automaton::core::io::bin2hex(res.second);
    hashes[hash]++;
    heights[hash] = res.first;
    // LOG(INFO) << "STATS end loop: " << i;
  }
  LOG(INFO) << "==== Heights ====";
  for (auto it = hashes.begin(); it != hashes.end(); ++it) {
    LOG(INFO) << "HASH: " << it->first << " AT HEIGHT: " << heights[it->first] << " #NODES: "
        << std::to_string(it->second);
  }
  LOG(INFO) << "=================";
  nodes_mutex.unlock();
  // LOG(INFO) << "STATS: 1";
}

void update_thread_function() {
  try {
    while (!simulation_end) {
      // LOG(INFO) << "UPDATE: 0";
      for (uint32_t i = 0; i < NUMBER_NODES && !simulation_end; ++i) {
        // LOG(INFO) << "UPDATE loop: " << i;
        nodes_mutex.lock();
        CHECK(nodes.size() == NUMBER_NODES) << "Number of nodes is not as expected! Have " <<
            nodes.size() << " expected " << NUMBER_NODES;
        node* n = nodes[i];
        nodes_mutex.unlock();
        n->update();
        // LOG(INFO) << "UPDATE end loop: " << i;
        // if (output_file.is_open()) {
        //   output_file << n->node_info();
        // } else {
        //   LOG(ERROR) << "Output file is closed";
        // }
        std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_STEP));
      }
      // LOG(INFO) << "UPDATE: 1";
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
}

void miner_thread_function() {
  try {
    while (!simulation_end) {
      // LOG(INFO) << "MINE: 0";
      for (uint32_t i = 0; i < NUMBER_NODES && !simulation_end; ++i) {
        // LOG(INFO) << "MINE loop " << i;
        nodes_mutex.lock();
        CHECK(nodes.size() == NUMBER_NODES) << "Number of nodes is not as expected! Have " <<
            nodes.size() << " expected " << NUMBER_NODES;
        node* n = nodes[i];
        nodes_mutex.unlock();
        n->mine(128, MINER_PRECISION_BITS);
        // LOG(INFO) << "MINE end loop " << i;
      }
      // LOG(INFO) << "MINE: 0";
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
}

void crash_handler(int sig) {
    LOG(ERROR) << "Woops! Crashed!";
    if (output_file.is_open()) {
      for (uint32_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->log_to_stream(output_file);
      }
    } else {
      LOG(ERROR) << "File is not open!";
    }
    el::Helpers::logCrashReason(sig, true);
    el::base::debug::StackTrace();
    el::Helpers::crashAbort(sig);
    simulation_end = true;
    miner.join();
    updater.join();
    automaton::core::network::tcp_release();
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      if (output_file.is_open()) {
        nodes[i]->log_to_stream(output_file);
      } else {
        LOG(ERROR) << "File is not open!";
      }
      delete nodes[i];
    }
    if (output_file.is_open()) {
      output_file.close();
    }
}

// ARGS:
// is_localhost->1 number_nodes my_min_port my_max_port
// not_localhost-> 0 number_nodes my_ip [other_ips]

int main(int argc, const char * argv[]) {
  el::Helpers::setCrashHandler(crash_handler);
  if (argc < 1) {
    LOG(ERROR) << "Number of provided arguments is not as expected!";
    return 0;
  }
  IS_LOCALHOST = std::stoi(argv[1]);

  LOG(DEBUG) << "ADDRESS: " << &IS_LOCALHOST;
  try {
    if (IS_LOCALHOST) {
      if (argc != 5) {
        LOG(ERROR) << "Number of provided arguments is not as expected!";
        return 0;
      }
      NUMBER_NODES = std::stoi(argv[2]);
      // MIN_PORT = std::stoi(argv[3]);
      // MAX_PORT = std::stoi(argv[4]);
      MY_MIN_PORT = std::stoi(argv[3]);
      MY_MAX_PORT = std::stoi(argv[4]);
      if (MY_MAX_PORT - MY_MIN_PORT + 1 < NUMBER_NODES) {
        LOG(ERROR) << "Not enough ports to run " << NUMBER_NODES << " nodes!";
        return 0;
      }
    } else {
      if (argc < 5) {
        LOG(ERROR) << "Number of provided arguments is not as expected!";
        return 0;
      }
      NUMBER_NODES = std::stoi(argv[2]);
      MY_IP = const_cast<char*>(argv[3]);
      // MIN_PORT = std::stoi(argv[3]);
      // MAX_PORT = std::stoi(argv[4]);
      if (MAX_PORT - MIN_PORT  + 1 < NUMBER_NODES) {
        LOG(ERROR) << "Not enough ports to run " << NUMBER_NODES << " nodes!";
        return 0;
      }
      for (int i = 4; i < argc; i++) {
        KNOWN_IPS.push_back(argv[i]);
      }
    }
    automaton::core::network::tcp_init();
    node::node_params params;
    params.connection_type = "tcp";
    params.connected_peers_count = NUMBER_PEERS_IN_NODE;
    params.timeout = TIMEOUT;
    LOG(INFO) << "Creating acceptors...";
    if (IS_LOCALHOST) {
      for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
        uint32_t tries = 5;
        std::string address;
        nodes.push_back(new node(params, create_localhost_acceptor_address,
            create_localhost_connection_address));
        if (!nodes[i]->init()) {
          std::stringstream s;
          s << "Node init failed!";
          LOG(ERROR) << s.str();
          throw std::runtime_error(s.str());
        }
        nodes[i]->set_id(std::to_string(i));
        do {
          address = create_localhost_acceptor_address(nodes[i], params);
        }  while (!nodes[i]->add_acceptor("tcp", address) && tries--);
      }
    } else {
      for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
        uint32_t tries = 5;
        std::string address;
        nodes.push_back(new node(params, create_remote_acceptor_address,
            create_remote_connection_address));
        if (!nodes[i]->init()) {
          std::stringstream s;
          s << "Node init failed!";
          LOG(ERROR) << s.str();
          throw std::runtime_error(s.str());
        }
        nodes[i]->set_id(std::to_string(i));
        do {
          address = create_remote_acceptor_address(nodes[i], params);
        }  while (!nodes[i]->add_acceptor("tcp", address) && tries--);
      }
    }
    CHECK(nodes.size() == NUMBER_NODES) << "Number of nodes is not correct!";
    LOG(INFO) << "Starting simulation...";
    updater = std::thread(update_thread_function);
    miner = std::thread(miner_thread_function);
    // ==============================================
    for (uint32_t i = 0; i < SIMULATION_TIME; i += LOOP_STEP) {
      LOG(INFO) << "PROCESSING: " + std::to_string(i);
      collect_stats();
      nodes_mutex.lock();
      for (uint32_t j = 0; j < nodes.size(); ++j) {
        std::string s = nodes[j]->node_info();
        // LOG(DEBUG) << "NODE " << nodes[j]->get_id() << " " << s;
        nodes[j]->add_to_log(s);
      }
      nodes_mutex.unlock();
      LOG(INFO) << "END OF PROCESSING: " + std::to_string(i);
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
  automaton::core::network::tcp_release();
  for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
    if (output_file.is_open()) {
      nodes[i]->log_to_stream(output_file);
    } else {
      LOG(ERROR) << "File is not open!";
    }
    delete nodes[i];
  }
  if (output_file.is_open()) {
    output_file.close();
  }
  return 0;
}
