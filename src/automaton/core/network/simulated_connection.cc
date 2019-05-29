#include "automaton/core/network/simulated_connection.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

#include "automaton/core/io/io.h"

using automaton::core::common::status;

namespace automaton {
namespace core {
namespace network {

// TODO(kari): Make thread safe.

connection_params::connection_params():min_lag(0), max_lag(0), bandwidth(0) {}
acceptor_params::acceptor_params():max_connections(0), bandwidth(0) {}

// SIMULATION

std::shared_ptr<simulation> simulation::simulator;

simulation::simulation():simulation_time(0), simulation_running(false) {
  connection::register_connection_type("sim", [](connection_id id, const std::string& address,
      std::shared_ptr<connection::connection_handler> handler) {
    return std::shared_ptr<connection>(new simulated_connection(id, address, handler));
  });
  acceptor::register_acceptor_type("sim", [](acceptor_id id, const std::string& address,
      std::shared_ptr<acceptor::acceptor_handler> handler,
      std::shared_ptr<connection::connection_handler> connections_handler) {
    return std::shared_ptr<acceptor>(new simulated_acceptor(id, address, handler, connections_handler));
  });
  std::srand(816405263);
}

simulation::~simulation() {
}

void simulation::simulation_start(uint64_t millisec_step) {
  if (millisec_step < 1) {
    millisec_step = 1;
  }
  running_mutex.lock();
  simulation_running = true;
  running_mutex.unlock();

  running_thread = std::thread([this, millisec_step](){
    uint64_t current = 0;
    while (true) {
      running_mutex.lock();
      if (simulation_running == false) {
          running_mutex.unlock();
          break;
      } else {
        running_mutex.unlock();
      }
      process(current);
      std::this_thread::sleep_for(std::chrono::milliseconds(millisec_step));
      current += millisec_step;
    }
  });

  handlers_thread = std::thread([this](){
    while (true) {
      running_mutex.lock();
      if (simulation_running == false) {
          running_mutex.unlock();
          break;
      } else {
        running_mutex.unlock();
      }
      process_handlers();
    }
  });
}

void simulation::simulation_stop() {
  running_mutex.lock();
  simulation_running = false;
  running_mutex.unlock();
  running_thread.join();
  handlers_thread.join();
}

void simulation::add_task(uint64_t tm, std::function<void()> task) {
  std::lock_guard<std::mutex> lock(tasks_mutex);
  std::lock_guard<std::mutex> time_lock(time_mutex);
  if (tm <= simulation_time) {
    tm = simulation_time + 1;
  }
  tasks[tm].push_back(task);
}

void simulation::add_handlers_task(std::function<void()> task) {
  std::lock_guard<std::mutex> lock(handlers_tasks_mutex);
  handlers_tasks.push(task);
}

std::shared_ptr<simulation> simulation::get_simulator() {
  if (simulator) {
    return simulator;
  }
  simulator = std::shared_ptr<simulation>(new simulation());
  return simulator;
}

void simulation::handle_disconnect(uint32_t dest) {
  // LOG(DBUG) << "disconnect 0";
  /**
    This event is created when the other endpoint has called disconnect().
    Sets connection state to disconnected.
  */
  std::shared_ptr<simulated_connection> destination =
      std::dynamic_pointer_cast<simulated_connection>(get_connection(dest));
  if (!destination) {  // state == disconnected should never happen
    LOG(INFO) << "Event disconnect but remote peer has already disconnected or does not exist";
    return;
  } else if (destination->get_state() == connection::state::connecting) {
    LOG(INFO) << "Event disconnect but peer is not connected yet! This situation is not handled right now!";
    return;
  }
  LOG(INFO) << "Other peer closed connection in: " << destination->get_address();
  destination->set_state(connection::state::disconnected);
  std::weak_ptr<connection::connection_handler> c_handler = destination->get_handler();
  auto c_id = destination->get_id();
  add_handlers_task([c_handler, c_id]() {
    std::shared_ptr<connection::connection_handler> handler = c_handler.lock();
    if (handler != nullptr) {
      handler->on_disconnected(c_id);
    }
  });
  destination->remote_connection_id = 0;
  add_task(get_time() + 1, [destination](){
    destination->cancel_operations();
    destination->clear_queues();
  });
  remove_connection(destination->local_connection_id);
  destination->local_connection_id = 0;
  // LOG(DBUG) << "disconnect 1";
}

void simulation::handle_request(uint32_t src, uint32_t dest) {
  // LOG(DBUG) << "attempt 0";
  /**
  This event is created when the other endpoint has called connect(). On_requested is called
  and if it returns true, new event with type accept is created, new connection is created
  from this endpoint to the other, the new connection's state is set to connected and
  on_connect in acceptor's handler is called. If the connections is
  refused, new event type refuse is created.
  **/
  std::shared_ptr<simulated_connection> source = std::dynamic_pointer_cast<simulated_connection>(get_connection(src));
  std::shared_ptr<simulated_acceptor> acceptor_ = std::dynamic_pointer_cast<simulated_acceptor>(get_acceptor(dest));
  if (!source) {
    LOG(ERROR) << "Connection request from unexisting peer: " << src;
    // TODO(kari): accept/refuse and then error
  }
  uint64_t time_of_handling = get_time() + source->get_lag() + 1;
  if (!acceptor_ || acceptor_->get_state() != acceptor::state::accepting) {
    LOG(ERROR) << "No such peer: " << dest;
    std::weak_ptr<connection::connection_handler> c_handler = source->get_handler();
    std::shared_ptr<simulation> sim = simulation::get_simulator();
    auto c_id = source->get_id();
    add_task(time_of_handling, [c_handler, c_id, sim]() {
      sim->add_handlers_task([c_handler, c_id]() {
        std::shared_ptr<connection::connection_handler> handler = c_handler.lock();
        if (handler != nullptr) {
          handler->on_connection_error(c_id, status::internal("No route to host"));
        }
      });
    });
  }
  std::string source_address = source->get_address();
  connection_id cid = 0;
  if (acceptor_->get_handler()->on_requested(acceptor_->get_id(), source_address, &cid)) {
    // LOG(DBUG) << "accepted";
    const connection_params& params = source->parameters;
    // Getting the smaller bandwidth from both sides
    auto bandwidth = acceptor_->parameters.bandwidth < source->parameters.bandwidth ?
        acceptor_->parameters.bandwidth : source->parameters.bandwidth;
    // Remote address of the other connection is 0 which means connect to that address is not possible.
    std::string new_addr = std::to_string(params.min_lag) + ":" + std::to_string(params.max_lag) + ":" +
        std::to_string(bandwidth) + ":0";
    std::shared_ptr<simulated_connection> new_connection(
        new simulated_connection(cid, new_addr, acceptor_->accepted_connections_handler));
    if (new_connection->init()) {
      add_connection(new_connection);
      source->parameters.bandwidth = new_connection->parameters.bandwidth = bandwidth;
      source->remote_connection_id = new_connection->local_connection_id;
      new_connection->set_state(connection::state::connected);
      new_connection->remote_connection_id = source->local_connection_id;
      new_connection->set_time_stamp(time_of_handling);
      std::weak_ptr<acceptor::acceptor_handler> a_handler = acceptor_->get_handler();
      auto a_id = acceptor_->get_id();
      add_handlers_task([a_handler, a_id, new_connection, source_address]() {
        std::shared_ptr<acceptor::acceptor_handler> handler = a_handler.lock();
        if (handler != nullptr) {
          handler->on_connected(a_id, new_connection, source_address);
        }
      });
      add_handlers_task([new_connection, cid](){
        new_connection->get_handler()->on_connected(cid);
      });
      add_task(get_time() + 1, [new_connection](){
        new_connection->handle_read();
      });
      add_task(get_time() + 1, [new_connection](){
        new_connection->handle_send();
      });
      std::shared_ptr<simulation> sim = get_simulator();
      add_task(time_of_handling, std::bind(&simulation::handle_accept, sim, src));
    } else {
      LOG(ERROR) << "Error while initializing connection";
    }
  } else {
    // LOG(DBUG) << "refused";
    std::shared_ptr<simulation> sim = get_simulator();
    add_task(time_of_handling, std::bind(&simulation::handle_refuse, sim, src));
  }
}

void simulation::handle_message(uint32_t src, uint32_t dest, const std::string& msg) {
  // LOG(DBUG) << "message 0";
  std::shared_ptr<simulated_connection> destination =
      std::dynamic_pointer_cast<simulated_connection>(get_connection(dest));
  std::shared_ptr<simulation> sim = get_simulator();
  std::shared_ptr<simulated_connection> source = std::dynamic_pointer_cast<simulated_connection>(get_connection(src));
  if (!destination || destination->get_state() != connection::state::connected) {
    LOG(ERROR) << "ERROR in handling send! Peer has disconnected or does not exist!";
    if (source) {
      uint32_t t = get_time();
      uint32_t ts = source->get_time_stamp();
      uint64_t time_of_handling = (t > ts ? t : ts) + 1 + source->get_lag();
      add_task(time_of_handling, std::bind(&simulation::handle_ack, sim, src, status::internal("Broken pipe")));
    }
    return;
  }
  destination->reading_q_mutex.lock();
  destination->recv_buf_mutex.lock();
  destination->receive_buffer.push(msg);
  if (destination->reading.size()) {
    add_task(get_time() + 1, std::bind(&simulated_connection::handle_read, destination));
  }
  destination->recv_buf_mutex.unlock();
  destination->reading_q_mutex.unlock();
  if (!source) {
    // LOG(DBUG) << "message 01";
    return;
  }
  source->sending_q_mutex.lock();
  if (source->sending.size() && source->sending.front().message.size() == source->sending.front().bytes_send) {
    source->sending_q_mutex.unlock();
    uint32_t t = get_time();
    uint32_t ts = destination->get_time_stamp();
    uint64_t time_of_handling = (t > ts ? t : ts) + 1 + destination->get_lag();
    destination->set_time_stamp(time_of_handling);
    std::shared_ptr<simulation> sim = get_simulator();
    add_task(time_of_handling, std::bind(&simulation::handle_ack, sim, src, status::ok()));
  } else if (source && source->get_state() == connection::state::connected) {
    add_task(get_time() + 1, std::bind(&simulated_connection::handle_send, source));
  }
  source->sending_q_mutex.unlock();
  // LOG(DBUG) << "message 1";
  return;
}

void simulation::handle_accept(uint32_t dest) {
  std::shared_ptr<simulated_connection> destination =
      std::dynamic_pointer_cast<simulated_connection>(get_connection(dest));
  if (!destination) {
    // new event error
    return;
  }
  destination->set_state(connection::state::connected);
  add_handlers_task([destination](){
    destination->get_handler()->on_connected(destination->get_id());
  });
  add_task(get_time() + 1, [destination](){
    destination->handle_read();
  });
  add_task(get_time() + 1, [destination](){
    destination->handle_send();
  });
}

void simulation::handle_refuse(uint32_t dest) {
  std::shared_ptr<simulated_connection> destination =
      std::dynamic_pointer_cast<simulated_connection>(get_connection(dest));
  // LOG(DBUG) << "refuse 0";
  if (!destination) {
    // new event error
    // LOG(DBUG) << "refuse 2";
    return;
  }
  destination->set_state(connection::state::disconnected);
  add_task(get_time() + 1, [destination](){
    destination->cancel_operations();
    destination->clear_queues();
  });
  add_handlers_task([destination](){
    destination->get_handler()->on_connection_error(destination->get_id(), status::internal("Connection refused!"));
  });
  // LOG(DBUG) << "refuse 1";
}

void simulation::handle_ack(uint32_t dest, const status& s) {
  // LOG(DBUG) << "<handle_ack>";
  std::shared_ptr<simulated_connection> destination =
      std::dynamic_pointer_cast<simulated_connection>(get_connection(dest));

  if (destination) {
    destination->sending_q_mutex.lock();
    if (destination->sending.size()) {
      uint32_t rid = destination->sending.front().id;
      destination->sending.pop();
      add_handlers_task([destination, rid, s](){
        destination->get_handler()->on_message_sent(destination->get_id(), rid, s);
      });
      add_task(get_time() + 1, [destination]() {
        destination->handle_send();
      });
    }
    destination->sending_q_mutex.unlock();
  }
}

uint32_t simulation::process(uint64_t time_) {
  // LOG(DBUG) << "process time: " << time_;
  int events_processed = 0;
  tasks_mutex.lock();

  auto current_time = get_time();
  // LOG(DBUG) << "Cur time: " << current_time << " process time: " << time_;
  while (current_time <= time_) {
    set_time(current_time);
    if (tasks.count(current_time)) {
      // LOG(DBUG) << "PROCESSING TIME: " << current_time;
      for (auto& t : tasks.at(current_time)) {
        events_processed++;
        tasks_mutex.unlock();
        try {
          t();
        } catch (const std::exception& e) {
          LOG(ERROR) << e.what();
        } catch (...) {
          LOG(ERROR) << "Error occured";
        }
        tasks_mutex.lock();
      }
      tasks.erase(current_time);
    }
    current_time++;
  }
  tasks_mutex.unlock();
  return events_processed;
}

void simulation::process_handlers() {
  handlers_tasks_mutex.lock();
  while (handlers_tasks.size() > 0) {
    running_mutex.lock();
    if (simulation_running == false) {
        running_mutex.unlock();
        break;
    } else {
      running_mutex.unlock();
    }
    auto t = handlers_tasks.front();
    handlers_tasks.pop();
    handlers_tasks_mutex.unlock();
    try {
      t();
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
    } catch (...) {
      LOG(ERROR) << "Error occured";
    }
    handlers_tasks_mutex.lock();
  }
  handlers_tasks_mutex.unlock();
}

uint64_t simulation::get_time() {
  std::lock_guard<std::mutex> lock(time_mutex);
  return simulation_time;
}

void simulation::set_time(uint64_t time_) {
  std::lock_guard<std::mutex> lock(time_mutex);
  if (time_ < simulation_time) {
    LOG(ERROR) << "Trying to set time < current time";
  } else {
    simulation_time = time_;
  }
}

bool simulation::is_queue_empty() {
  std::lock_guard<std::mutex> lock(tasks_mutex);
  return tasks.empty();
}

void simulation::add_connection(std::shared_ptr<connection> connection_) {
  static uint32_t uid = 0;
  if (connection_) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    auto lid = std::dynamic_pointer_cast<simulated_connection>(connection_)->local_connection_id;
    if (lid) {
      auto it = connections.find(lid);
      if (it != connections.end() && it->second != connection_) {
        connections.erase(it);
      }
    }
    connections[++uid] = connection_;
    std::dynamic_pointer_cast<simulated_connection>(connection_)->local_connection_id = uid;
  }
}

void simulation::remove_connection(uint32_t connection_id) {
  std::lock_guard<std::mutex> lock(connections_mutex);
  auto iterator_ = connections.find(connection_id);
  if (iterator_ != connections.end()) {
    connections.erase(iterator_);
  }
}

std::shared_ptr<connection> simulation::get_connection(uint32_t cid) {
  std::lock_guard<std::mutex> lock(connections_mutex);
  auto iterator_ = connections.find(cid);
  if (iterator_ == connections.end()) {
    return nullptr;
  }
  return std::shared_ptr<connection>(iterator_->second);
}

void simulation::add_acceptor(uint32_t address, std::shared_ptr<acceptor> acceptor_) {
  std::lock_guard<std::mutex> lock(acceptors_mutex);
  acceptors[address] = acceptor_;
}

std::shared_ptr<acceptor> simulation::get_acceptor(uint32_t address) {
  std::lock_guard<std::mutex> lock(acceptors_mutex);
  auto iterator_ = acceptors.find(address);
  if (iterator_ == acceptors.end()) {
    return nullptr;
  }
  return iterator_->second;
}

void simulation::remove_acceptor(uint32_t address) {
  std::lock_guard<std::mutex> lock(acceptors_mutex);
  auto iterator_ = acceptors.find(address);
  if (iterator_ != acceptors.end()) {
    acceptors.erase(iterator_);
  }
}

// CONNECTION

simulated_connection::incoming_packet::incoming_packet(): buffer(nullptr), buffer_size(0),
    expect_to_read(0), id(0), bytes_read(0) {}

simulated_connection::outgoing_packet::outgoing_packet(): bytes_send(0), id(0) {}

simulated_connection::simulated_connection(connection_id id, const std::string& address_,
    std::shared_ptr<connection_handler> handler_):
    connection(id, handler_), remote_address(0), local_connection_id(0), remote_connection_id(0), time_stamp(0),
    original_address(address_), connection_state(connection::state::invalid_state) {
}

simulated_connection::~simulated_connection() {
}

bool simulated_connection::init() {
  connection_state = connection::state::disconnected;
  if (!parse_address(original_address, &parameters, &remote_address)) {
    std::stringstream msg;
    LOG(ERROR) << "ERROR: Connection creation failed! Could not resolve address and parameters in: "
        << original_address;
    return false;
  }
  return true;
}

void simulated_connection::async_send(const std::string& message, uint32_t msg_id = 0) {
  // LOG(DBUG) << id << " <async_send>";
  if (message.size() < 1) {
    LOG(ERROR) << "Send called but no message: id -> " << msg_id;
    // LOG(DBUG) << id << " </async_send>";
    return;
  }
  if (get_state() == disconnected) {
    LOG(ERROR) << "Cannot send message! Call connect first!";
    // LOG(DBUG) << id << " </async_send>";
    return;
  }
  // LOG(DBUG) << "Send called with message <" << io::bin2hex(message) << ">";
  outgoing_packet packet;
  packet.message = message;
  packet.bytes_send = 0;
  packet.id = msg_id;
  // LOG(DBUG) << id << " pushing message <" << io::bin2hex(message) << "> with id: " << msg_id;
  sending_q_mutex.lock();
  sending.push(std::move(packet));
  sending_q_mutex.unlock();
  if (get_state() == connection::state::connected) {
    std::shared_ptr<simulation> sim = simulation::get_simulator();
    sim->add_task(sim->get_time() + 1, std::bind(&simulated_connection::handle_send, shared_from_this()));
  }
  // LOG(DBUG) << id << " </async_send>";
}

void simulated_connection::handle_send() {
  // LOG(DBUG) << id << " <handle_send>";
  if (get_state() != connection::state::connected) {
    return;
  }
  sending_q_mutex.lock();
  /// nothing to send or waiting for ACK
  if (sending.empty() || sending.front().message.size() == sending.front().bytes_send) {
    sending_q_mutex.unlock();
    // LOG(DBUG) << id << " </handle_send 1 >";
    return;
  }
  outgoing_packet& packet = sending.front();
  sending_q_mutex.unlock();
  std::string message = "";
  if (parameters.bandwidth >= (packet.message.size() - packet.bytes_send)) {
    if (!packet.bytes_send) {
      message = packet.message;
    } else {
      message = packet.message.substr(packet.bytes_send);
    }
  } else {
      message = packet.message.substr(packet.bytes_send, parameters.bandwidth);
  }
  packet.bytes_send += message.size();
  std::shared_ptr<simulation> sim = simulation::get_simulator();
  uint32_t t = sim->get_time();
  uint32_t ts = get_time_stamp();
  uint64_t time_of_handling = (t > ts ? t : ts) + 1 + get_lag();
  set_time_stamp(time_of_handling);
  sim->add_task(time_of_handling,
      std::bind(&simulation::handle_message, sim, local_connection_id, remote_connection_id, message));
  // LOG(DBUG) << "</handle_send 2>";
}

void simulated_connection::handle_read() {
  // LOG(DBUG) << "<handle_read>";
  if (get_state() != connection::state::connected) {
    return;
  }
  reading_q_mutex.lock();
  recv_buf_mutex.lock();
  if (receive_buffer.size() && reading.size()) {
    incoming_packet& packet = reading.front();
    bool read_some = packet.expect_to_read == 0;
    uint32_t max_to_read = read_some ? packet.buffer_size : packet.expect_to_read;
    while (receive_buffer.size() && packet.bytes_read < max_to_read) {
      uint32_t left_to_read = max_to_read - packet.bytes_read;
      std::string message = receive_buffer.front();
      if (left_to_read >= message.size()) {
        std::memcpy(packet.buffer.get() + packet.bytes_read, message.data(), message.size());
        packet.bytes_read += message.size();
        receive_buffer.pop();
      } else {
        std::memcpy(packet.buffer.get() + packet.bytes_read, message.data(), left_to_read);
        packet.bytes_read += left_to_read;
        receive_buffer.front() = message.substr(left_to_read);
      }
    }
    if (read_some || packet.bytes_read == packet.expect_to_read) {
      incoming_packet packet = std::move(reading.front());
      reading.pop();
      auto self = shared_from_this();
      std::shared_ptr<simulation> sim = simulation::get_simulator();
      sim->add_task(sim->get_time() + 1, std::bind(&simulated_connection::handle_read, self));
      sim->add_handlers_task([self, packet]() {
        self->handler->on_message_received(self->id, packet.buffer, packet.bytes_read, packet.id);
      });
    }
  }
  recv_buf_mutex.unlock();
  reading_q_mutex.unlock();
  // LOG(DBUG) << id << " </handle_read 1>";
}

void simulated_connection::async_read(std::shared_ptr<char> buffer, uint32_t buffer_size, uint32_t num_bytes = 0,
    uint32_t rid = 0) {
  /**
    This function does not create event. Actual reading will happen when the
    other endpoint sends a message and it arrives (send event is handled and
    read event is created).
  */
  // TODO(kari): If disconnected, return
  if (num_bytes > buffer_size) {
    LOG(ERROR) << id << " ERROR: Buffer size " << buffer_size << " is smaller than needed (" <<
        num_bytes << ")! Reading aborted!";
    return;
  }
  //  LOG(DBUG) << id << " Setting buffers in connection: " << get_address();
  incoming_packet packet;
  packet.buffer = buffer;
  packet.buffer_size = buffer_size;
  packet.expect_to_read = num_bytes;
  packet.id = rid;
  reading_q_mutex.lock();
  reading.push(std::move(packet));
  reading_q_mutex.unlock();
  recv_buf_mutex.lock();
  if (receive_buffer.size()) {
    std::shared_ptr<simulation> sim = simulation::get_simulator();
    sim->add_task(sim->get_time() + 1, std::bind(&simulated_connection::handle_read, shared_from_this()));
  }
  recv_buf_mutex.unlock();
  // LOG(DBUG) << id  << " </async_read>";
}

bool simulated_connection::parse_address(const std::string& address_, connection_params* params,
    uint32_t* parsed_remote_address) {
  std::regex rgx_sim("(\\d+):(\\d+):(\\d+):(\\d+)");
  std::smatch match;
  if (std::regex_match(address_.begin(), address_.end(), match, rgx_sim) &&
      std::stoul(match[1]) <= std::stoul(match[2]) &&
      match.size() == 5) {
    params->min_lag = std::stoul(match[1]);
    params->max_lag = std::stoul(match[2]);
    params->bandwidth = std::stoul(match[3]);
    *parsed_remote_address = std::stoul(match[4]);
    return true;
  }
  return false;
}

connection::state simulated_connection::get_state() const {
  std::lock_guard<std::mutex> lock(state_mutex);
  return connection_state;
}

void simulated_connection::set_state(connection::state new_state) {
  std::lock_guard<std::mutex> lock(state_mutex);
  connection_state = new_state;
}

std::string simulated_connection::get_address() const {
  return "conn_id:" + std::to_string(local_connection_id) +
      "/remote_conn_id:" + std::to_string(remote_connection_id) +
      "/accptr:" + std::to_string(remote_address);
}

uint32_t simulated_connection::get_lag() const {
  if (parameters.min_lag == parameters.max_lag) {
    return parameters.min_lag;
  }
  return std::rand() % (parameters.max_lag - parameters.min_lag) + parameters.min_lag;
}

void simulated_connection::connect() {
  if (!remote_address) {
    LOG(ERROR) << id << " Cannot connect: No address to connect to!";
    return;
  }
  // LOG(DBUG) << id << " <connect>";
  connection_state = connection::state::connecting;
  std::shared_ptr<simulation> sim = simulation::get_simulator();
  sim->add_connection(shared_from_this());
  uint32_t t = sim->get_time();
  uint32_t ts = get_time_stamp();
  uint64_t time_of_handling = (t > ts ? t : ts) + 1 + get_lag();
  set_time_stamp(time_of_handling);
  // LOG(DEBUG) << id << " </connect>";

  sim->add_task(time_of_handling, std::bind(&simulation::handle_request, sim, local_connection_id, remote_address));
}

void simulated_connection::disconnect() {
    // LOG(DBUG) << id << " <disconnect>";
    if (connection_state != connection::state::connected) {
      return;
    }
    connection_state = connection::state::disconnected;
    std::shared_ptr<simulation> sim = simulation::get_simulator();
    uint32_t t = sim->get_time();
    uint32_t ts = get_time_stamp();
    uint64_t time_of_handling = (t > ts ? t : ts) + 1 + get_lag();
    set_time_stamp(time_of_handling);
    sim->remove_connection(local_connection_id);
    local_connection_id = 0;
    auto self = shared_from_this();
    sim->add_task(sim->get_time() + 1, [self](){
      self->cancel_operations();
      self->clear_queues();
    });
    std::weak_ptr<connection::connection_handler> c_handler = handler;
    auto c_id = id;
    sim->add_handlers_task([c_handler, c_id]() {
      std::shared_ptr<connection::connection_handler> handler = c_handler.lock();
      if (handler != nullptr) {
        handler->on_disconnected(c_id);
      }
    });
    // LOG(DBUG) << id << " </disconnect>";
    sim->add_task(time_of_handling, std::bind(&simulation::handle_disconnect, sim, remote_connection_id));
    remote_connection_id = 0;
}

std::shared_ptr<connection::connection_handler> simulated_connection::get_handler() {
  return handler;
}

void simulated_connection::set_time_stamp(uint32_t t) {
  std::lock_guard<std::mutex> lock(time_stamp_mutex);
  if (t > time_stamp) {
    time_stamp = t;
  }
}

uint32_t simulated_connection::get_time_stamp() const {
  std::lock_guard<std::mutex> lock(time_stamp_mutex);
  return time_stamp;
}

void simulated_connection::clear_queues() {
  LOG(DBUG) << id << " Clearing queues";
  std::lock_guard<std::mutex> sending_lock(sending_q_mutex);
  std::lock_guard<std::mutex> reading_lock(reading_q_mutex);
  std::lock_guard<std::mutex> recv_lock(recv_buf_mutex);
  std::queue<incoming_packet> empty_reading;
  std::swap(reading, empty_reading);
  std::queue<outgoing_packet> empty_sending;
  std::swap(sending, empty_sending);
  std::queue<std::string> empty_receive_buffer;
  std::swap(receive_buffer, empty_receive_buffer);
}

void simulated_connection::cancel_operations() {
  LOG(DBUG) << id << " Canceling operations";
  std::lock_guard<std::mutex> sending_lock(sending_q_mutex);
  std::lock_guard<std::mutex> reading_lock(reading_q_mutex);
  std::shared_ptr<connection::connection_handler> c_handler = handler;
  connection_id c_id = id;
  std::shared_ptr<simulation> sim = simulation::get_simulator();
  while (!sending.empty()) {
    auto m_id = sending.front().id;
    sim->add_handlers_task([c_handler, c_id, m_id]() {
      c_handler->on_message_sent(c_id, m_id, status::aborted("Operation cancelled!"));
    });
    sending.pop();
  }
  uint32_t n = reading.size();
  while (n--) {
    sim->add_handlers_task([c_handler, c_id]() {
      c_handler->on_connection_error(c_id, status::aborted("Operation read cancelled!"));
    });
  }
}

// ACCEPTOR

simulated_acceptor::simulated_acceptor(acceptor_id id, const std::string& address_,
    std::shared_ptr<acceptor::acceptor_handler> handler,
    std::shared_ptr<connection::connection_handler> connections_handler):
    acceptor(id, handler), address(0), original_address(address_),
    accepted_connections_handler(connections_handler), acceptor_state(acceptor::state::invalid_state) {
}

simulated_acceptor::~simulated_acceptor() {
}

bool simulated_acceptor::init() {
  if (parse_address(original_address, &parameters, &address)) {
    if (!address) {
      LOG(ERROR) << "ERROR: Acceptor creation failed! Acceptor address should be > 0";
      return false;
    } else {
      simulation::get_simulator()->add_acceptor(address, shared_from_this());
    }
  } else {
    LOG(ERROR) << "ERROR: Acceptor creation failed! Could not resolve address and parameters in: " << original_address;
    return false;
  }
  set_state(acceptor::state::not_accepting);
  return true;
}

bool simulated_acceptor::parse_address(const std::string& address_, acceptor_params* params, uint32_t* parsed_address) {
  std::regex rgx_sim("(\\d+):(\\d+):(\\d+)");
  std::smatch match;
  if (std::regex_match(address_.begin(), address_.end(), match, rgx_sim) && match.size() == 4) {
    params->max_connections = std::stoul(match[1]);
    params->bandwidth = std::stoul(match[2]);
    *parsed_address = std::stoul(match[3]);
    return true;
  }
  return false;
}

void simulated_acceptor::start_accepting() {
  if (!address) {
    LOG(ERROR) << "This should never happen! Acceptor's address is not valid! Could not accept";
  }
  set_state(acceptor::state::accepting);
}

void simulated_acceptor::stop_accepting() {
  set_state(acceptor::state::not_accepting);
}

acceptor::state simulated_acceptor::get_state() const {
  std::lock_guard<std::mutex> lock(state_mutex);
  return acceptor_state;
}

void simulated_acceptor::set_state(acceptor::state new_state) {
  std::lock_guard<std::mutex> lock(state_mutex);
  acceptor_state = new_state;
}

std::string simulated_acceptor::get_address() const {
  return original_address;
}

std::shared_ptr<acceptor::acceptor_handler> simulated_acceptor::get_handler() {
  return handler;
}

}  // namespace network
}  // namespace core
}  // namespace automaton
