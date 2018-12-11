#include <cstdlib>
#include <mutex>
#include <string>
#include <sstream>

#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"

using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::core::network::simulation;

std::mutex buffer_mutex;
std::vector<char*> buffers;

char* add_buffer(uint32_t size) {
  std::lock_guard<std::mutex> lock(buffer_mutex);
  buffers.push_back(new char[size]);
  return buffers[buffers.size() - 1];
}
void clear_buffers() {
  std::lock_guard<std::mutex> lock(buffer_mutex);
  for (uint32_t i = 0; i < buffers.size(); ++i) {
    delete [] buffers[i];
  }
}

/// Constants

static const uint32_t NUMBER_NODES = 100000;
// These include only the peers that a node connects to, not the accepted ones
static const uint32_t NUMBER_PEERS_IN_NODE = 4;
static const uint32_t MIN_LAG = 100;
static const uint32_t MAX_LAG = 1000;
static const uint32_t MIN_CONNECTIONS = 0;
static const uint32_t MAX_CONNECTIONS = 1;
static const uint32_t MIN_BANDWIDTH = 16;
static const uint32_t MAX_BANDWIDTH = 16;
static const uint32_t LOOP_STEP = 100;
static const uint32_t PROCESS_STEP = 100;
static const uint32_t BLOCK_CREATION_STEP = 1500;
static const uint32_t MAX_SIMULATION_TIME = 10000;

/// Global variables

class node;
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

/// Class node
class node {
 public:
  uint32_t height;
  acceptor* acceptor_;
  connection::connection_handler* handler_;
  std::vector<connection*> peers;
  void send_height() {
    for (uint32_t i = 0; i < peers.size(); ++i) {
      peers[i]->async_send(std::to_string(height), 0);
    }
  }
};

/// Connection handler
class handler: public connection::connection_handler {
 public:
  node* node_;
  explicit handler(node* n): node_(n) {}
  void on_message_received(connection* c, char* buffer, uint32_t bytes_read, uint32_t id) {
    std::string message = std::string(buffer, bytes_read);
    // logging("Message \"" + message + "\" received in <" + c->get_address() + ">");
    if (std::stoul(message) > node_->height) {
      node_->height = std::stoul(message);
      node_->send_height();
    }
    c -> async_read(buffer, 16, 0, 0);
  }
  void on_message_sent(connection* c, uint32_t id, connection::error e) {
    if (e) {
       LOG(ERROR) << "Message with id " << std::to_string(id) << " was NOT sent to " <<
          c->get_address() << "\nError " << std::to_string(e);
    } else {
      // logging("Message with id " + std::to_string(id) + " was successfully sent to " +
      //    c->get_address());
    }
  }
  void on_connected(connection* c) {
    // logging("Connected with: " + c->get_address());
  }
  void on_disconnected(connection* c) {
    // logging("Disconnected with: " + c->get_address());
  }
  void on_error(connection* c, connection::error e) {
    if (e == connection::no_error) {
      return;
    }
    LOG(ERROR) << "Error: " << std::to_string(e) << " (connection " << c->get_address() << ")";
  }
};

/// Acceptor handler
class lis_handler: public acceptor::acceptor_handler {
 public:
  node* node_;
  explicit lis_handler(node* n):node_(n) {}
  bool on_requested(acceptor* a, const std::string& address) {
    // EXPECT_EQ(address, address_a);
    // logging("Connection request from: " + address + ". Accepting...");
    return true;
  }
  void on_connected(acceptor* a, connection* c, const std::string& address) {
    // logging("Accepted connection from: " + address);
    c->async_read(add_buffer(16), 16, 0, 0);
    node_->peers.push_back(c);
  }
  void on_error(acceptor* a, connection::error e) {
    LOG(ERROR) << std::to_string(e);
  }
};

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
      nodes[i]->height = 0;
      nodes[i]->handler_ = new handler(nodes[i]);
      nodes[i]->acceptor_ =
          acceptor::create("sim", create_acceptor_address(i+1), new lis_handler(nodes[i]),
                          nodes[i]->handler_);
      nodes[i]->acceptor_->start_accepting();
    }
    LOG(INFO) << "Creating connections...";
    for (uint32_t i = 0; i < NUMBER_NODES; ++i) {
      for (uint32_t j = 0; j < NUMBER_PEERS_IN_NODE; ++j) {
        connection* new_connection = connection::create("sim",
                                                        create_connection_address(NUMBER_NODES, i),
                                                        nodes[i]->handler_);
        nodes[i]->peers.push_back(new_connection);
        new_connection->connect();
        new_connection->async_read(add_buffer(16), 16, 0, 0);
      }
    }
    LOG(INFO) << "Starting simulation...";
    // ==============================================
    for (uint32_t i = 0; i < MAX_SIMULATION_TIME; i += LOOP_STEP) {
      if (i % PROCESS_STEP == 0) {
        LOG(INFO) << "PROCESSING: " + std::to_string(i);
        // sim->print_connections();
        int events_processed = sim->process(i);
        LOG(INFO) << "Events processed: " << events_processed;
        if ((i+LOOP_STEP) % BLOCK_CREATION_STEP == 0) {
          int n = std::rand() % NUMBER_NODES;
          ++nodes[n]->height;
          nodes[n]->send_height();
        }
      }
      collect_stats();
    }
    // sim->print_q();
  } catch (std::exception& e) {
    LOG(ERROR) << "EXCEPTION " + std::string(e.what());
  } catch(...) {
    LOG(ERROR) << "UNKOWN EXCEPTION!";
  }
  clear_buffers();
  return 0;
}
