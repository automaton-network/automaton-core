#ifndef AUTOMATON_CORE_NETWORK_SIMULATED_CONNECTION_H_
#define AUTOMATON_CORE_NETWORK_SIMULATED_CONNECTION_H_

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "automaton/core/network/acceptor.h"
#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace network {

class simulated_connection;
class simulated_acceptor;

/**
  Acceptor address: (max connections):(bandwidth):(address)
  Connection address: (min lag):(max lag):(bandwidth):(remote address)
*/

// these could be protobuf messages
struct connection_params {
  uint32_t min_lag;
  uint32_t max_lag;
  uint32_t bandwidth;
  connection_params();
};

struct acceptor_params {
  uint32_t max_connections;
  uint32_t bandwidth;
  acceptor_params();
};

/**
  Singleton class running the simulation. Stores created acceptors, connections and simulation time. Handle event queue.
*/
class simulation {
 private:
  /**
    Map storing created connections. Ids are unique for the simulation.
  */
  std::unordered_map<uint32_t, std::shared_ptr<connection> > connections;
  std::mutex connections_mutex;

  /**
    Map storing created acceptors where the key is the address.
  */
  std::unordered_map<uint32_t, std::shared_ptr<acceptor> > acceptors;
  std::mutex acceptors_mutex;

  /**
    Priority queue storing the events that need to be handled at specific time. Lower time means higher priority.
    If equal, FIFO.
  */
  std::mutex tasks_mutex;
  std::unordered_map<uint64_t, std::vector<std::function<void()> > > tasks;

  /**
    Simulation time. On create is 0.
  */
  uint64_t simulation_time;
  std::mutex time_mutex;

  /** The simulation instance.*/
  static std::shared_ptr<simulation> simulator;

  // Constructor.
  simulation();

  /** Update the simulation time.*/
  void set_time(uint64_t time);

  bool simulation_running;
  std::thread running_thread;
  std::mutex running_mutex;

 public:
  ~simulation();

  // TODO(kari): Make it work on exactly millisec_step milliseconds.
  /**
    If called, process will be invoked every *millisec_step* milliseconds + the process time. New thread is started.
    Simulation_stop should be called so the thread can be stopped and joined.
  */
  void simulation_start(uint64_t millisec_step);

  // TODO(kari): Move this to simulation destructor.
  /**
    Stops the simulation and joins the process thread. Needs to be called at the end of the program.
  */
  void simulation_stop();

  /**
    Returns pointer to the simulation instance;
  */
  static std::shared_ptr<simulation> get_simulator();

  void add_task(uint64_t tm, std::function<void()> task);

  /** Returns current simulation time */
  uint64_t get_time();

  /** Checks if the event/task queue is empty. */
  bool is_queue_empty();

  /**
    Process all events from simulation_time to the given time.
  */
  uint32_t process(uint64_t time);

  // NOTE: This should be called only from simulation and simulated_connection. Should not be public but it is for now.
  void add_connection(std::shared_ptr<connection> connection_);

  std::shared_ptr<connection> get_connection(uint32_t connection_id);

  void remove_connection(uint32_t connection_id);

  // NOTE: This is called only from acceptor. Should not be public but it is for now.
  void add_acceptor(uint32_t address, std::shared_ptr<acceptor> acceptor_);

  std::shared_ptr<acceptor> get_acceptor(uint32_t address);

  void remove_acceptor(uint32_t address);

  void handle_disconnect(uint32_t dest);

  void handle_request(uint32_t src, uint32_t dest);

  void handle_message(uint32_t source, uint32_t destination, const std::string& message);

  void handle_accept(uint32_t dest);

  void handle_refuse(uint32_t dest);

  void handle_ack(uint32_t dest, const automaton::core::common::status& s);
};

class simulated_connection: public connection, public std::enable_shared_from_this<simulated_connection> {
 public:
  uint32_t remote_address;
  uint32_t local_connection_id;
  uint32_t remote_connection_id;

  struct incoming_packet {
    std::shared_ptr<char> buffer;
    uint32_t buffer_size;
    uint32_t expect_to_read;
    uint32_t id;
    uint32_t bytes_read;
    incoming_packet();
  };

  struct outgoing_packet {
    std::string message;
    uint32_t bytes_send;
    uint32_t id;
    outgoing_packet();
  };

  connection_params parameters;

  std::queue<outgoing_packet> sending;
  std::mutex sending_q_mutex;
  std::queue<incoming_packet> reading;
  std::mutex reading_q_mutex;
  std::queue<std::string> receive_buffer;
  std::mutex recv_buf_mutex;

  simulated_connection(connection_id id, const std::string& address_, std::shared_ptr<connection_handler> handler_);

  ~simulated_connection();

  bool init();

  bool parse_address(const std::string& address_, connection_params* params, uint32_t* parsed_remote_address);

  void connect();

  void disconnect();

  void async_send(const std::string& message, uint32_t message_id);

  void async_read(std::shared_ptr<char> buffer, uint32_t buffer_size, uint32_t num_bytes, uint32_t id);

  void handle_read();
  void handle_send();

  state get_state() const;

  void set_state(connection::state new_state);

  std::string get_address() const;

  uint32_t get_lag() const;

  std::shared_ptr<connection_handler> get_handler();

  void set_time_stamp(uint32_t t);

  uint32_t get_time_stamp() const;

  /**
    Clears queues related to read operation. It is used when disconnecting.
    Could be removed if connections are unique and won't be reused.
  */
  void clear_queues();

  void cancel_operations();

 private:
  /**
    This is used when setting event time to prevent events that are called before others to be
    handled first because they had smaller lag. It shows the last time when THIS endpoint send
    something to the other (message, acknowlede). When the other send acknowlede of some kind,
    this time_stamp is NOT taken into account and it is possible 2 events for this connection to be
    in the event queue at the same time.
  */
  uint32_t time_stamp;  // time_stamp
  mutable std::mutex time_stamp_mutex;
  std::string original_address;
  connection::state connection_state;
  mutable std::mutex state_mutex;
};

class simulated_acceptor: public acceptor, public std::enable_shared_from_this<simulated_acceptor> {
 public:
  uint32_t address;
  std::string original_address;
  acceptor_params parameters;
  std::shared_ptr<connection::connection_handler> accepted_connections_handler;

  simulated_acceptor(acceptor_id id, const std::string& address_, std::shared_ptr<acceptor::acceptor_handler>
      handler_, std::shared_ptr<connection::connection_handler> accepted_connections_handler);

  ~simulated_acceptor();

  bool init();

  void start_accepting();

  void stop_accepting();

  state get_state() const;

  void set_state(acceptor::state new_state);

  std::string get_address() const;

  bool parse_address(const std::string& address_, acceptor_params* params, uint32_t* parsed_address);

  std::shared_ptr<acceptor_handler> get_handler();

 private:
  acceptor::state acceptor_state;
  mutable std::mutex state_mutex;
};

}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_SIMULATED_CONNECTION_H_
