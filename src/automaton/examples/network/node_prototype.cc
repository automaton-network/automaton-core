#include "automaton/examples/network/node_prototype.h"

#include "automaton/core/io/io.h"

namespace acn = automaton::core::network;

/// Node's connection handler

node::handler::handler(node* n): node_(n) {}
void node::handler::on_message_received(acn::connection* c, char* buffer,
    uint32_t bytes_read, uint32_t id) {
  std::string message = std::string(buffer, bytes_read);
  // logging("Message \"" + message + "\" received in <" + c->get_address() + ">");
  if (std::stoul(message) > node_->height) {
    node_->height = std::stoul(message);
    node_->send_height();
  }
  c -> async_read(buffer, 16, 0, 0);
}
void node::handler::on_message_sent(acn::connection* c, uint32_t id,
    acn::connection::error e) {
  if (e) {
     LOG(ERROR) << "Message with id " << std::to_string(id) << " was NOT sent to " <<
        c->get_address() << "\nError " << std::to_string(e);
  } else {
    // logging("Message with id " + std::to_string(id) + " was successfully sent to " +
    //    c->get_address());
  }
}
void node::handler::on_connected(acn::connection* c) {
  c->async_read(node_->add_buffer(16), 16, 0, 0);
}
void node::handler::on_disconnected(acn::connection* c) {
  // logging("Disconnected with: " + c->get_address());
}
void node::handler::on_error(acn::connection* c,
    acn::connection::error e) {
  if (e == acn::connection::no_error) {
    return;
  }
  LOG(ERROR) << "Error: " << std::to_string(e) << " (connection " << c->get_address() << ")";
}

/// Node's acceptor handler

node::lis_handler::lis_handler(node* n):node_(n) {}
bool node::lis_handler::on_requested(acn::acceptor* a, const std::string& address) {
  // EXPECT_EQ(address, address_a);
  // logging("Connection request from: " + address + ". Accepting...");
  return node_->accept_connection(/*address*/);
}
void node::lis_handler::on_connected(acn::acceptor* a, acn::connection* c,
    const std::string& address) {
  // logging("Accepted connection from: " + address);
  node_->peers[node_->get_next_peer_id()] = c;
  c->async_read(node_->add_buffer(16), 16, 0, 0);
}
void node::lis_handler::on_error(acn::acceptor* a, acn::connection::error e) {
  LOG(ERROR) << std::to_string(e);
}

/// Node

node::node() {
  height = 0;
  peer_ids = 0;
  handler_ = new handler(this);
  lis_handler_ = new lis_handler(this);
}
node::~node() {
  for (int i = 0; i < buffers.size(); ++i) {
    delete [] buffers[i];
  }
  // TODO(kari): delete all acceptors and connections
}

char* node::add_buffer(uint32_t size) {
  std::lock_guard<std::mutex> lock(buffer_mutex);
  buffers.push_back(new char[size]);
  return buffers[buffers.size() - 1];
}
/// This function is created because the acceptor needs ids for the connections it accepts
uint32_t node::get_next_peer_id() {
  return ++peer_ids;
}
bool node::accept_connection() { return true; }
bool node::add_peer(uint32_t id, const std::string& connection_type, const std::string& address) {
  auto it = peers.find(id);
  if (it != peers.end()) {
  //  delete it->second;  /// Delete existing acceptor
  }
  acn::connection* new_connection;
  try {
    new_connection = acn::connection::create(connection_type, address,
        handler_);
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    peers[id] = nullptr;
    return false;
  }
  if (!new_connection) {
    LOG(ERROR) << "Connection was not created!";  // Possible reason: tcp_init was never called
    return false;
  }
  peers[id] = new_connection;
  new_connection->connect();
  return true;
}
void node::remove_peer(uint32_t id) {
  auto it = peers.find(id);
  if (it != peers.end()) {
    peers.erase(it);
  }
}
bool node::add_acceptor(uint32_t id, const std::string& connection_type,
    const std::string& address) {
  auto it = acceptors.find(id);
  if (it != acceptors.end()) {
    // delete it->second;  /// Delete existing acceptor
  }
  acn::acceptor* new_acceptor;
  try {
    new_acceptor = acn::acceptor::create(connection_type, address,
        lis_handler_, handler_);
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    acceptors[id] = nullptr;
    return false;
  }
  if (!new_acceptor) {
    LOG(ERROR) << "Acceptor was not created!";
    return false;
  }
  acceptors[id] = new_acceptor;
  new_acceptor->start_accepting();
  return true;
}
void node::remove_acceptor(uint32_t id) {
  auto it = acceptors.find(id);
  if (it != acceptors.end()) {
    acceptors.erase(it);
  }
}
void node::send_height(uint32_t connection_id) {
  if (!connection_id) {
    for (auto it = peers.begin(); it != peers.end(); ++it) {
      // TODO(kari): if connected
      it->second->async_send(std::to_string(height), 0);
    }
  } else {
    peers[connection_id]->async_send(std::to_string(height), 0);
  }
}
