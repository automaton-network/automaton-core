#ifndef AUTOMATON_CORE_NETWORK_TCP_IMPLEMENTATION_H_
#define AUTOMATON_CORE_NETWORK_TCP_IMPLEMENTATION_H_

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <boost/config/warning_disable.hpp>

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include "automaton/core/network/acceptor.h"
#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace network {

// TODO(kari): proper exit, clear resources..
// TODO(kari): add worker and init function

/*
  the variables below may be used in handler:
  on_connected() {
    if error && #try < #tries_to_connect
    ...
  }
  on_message_sent() {
    if couldn't sent && #try < #tries_send
      try again  ...
  }
int number_tries_connect = 3;
int number_tries_send = 3;
int number_tries_receive = 3;
int max_connections = 8;

*/

/**
  Class that represents a connection with a remote peer.
*/
class tcp_connection: public connection, public std::enable_shared_from_this<tcp_connection> {
 public:
  /**
    Constructor that will be used when this class is registered.

    @see connection::create

    @param[in] address_ string representing the address of the connection - ip:port
  */
  tcp_connection(connection_id id, const std::string& address_, std::shared_ptr<connection_handler> handler_);

  /**
    Constructor that should be used **ONLY** from the acceptor. It is used when there is a connection request and
      the user has accepted it.
      @see acceptor::acceptor_handler::on_requested
      @see acceptor::acceptor_handler::on_connected

    @param[in] id identifier given by the user. It is used in connection_handler's functions to specify the connection
      invoked the function.
    @param[in] address_ string representing the address of the connection - ip:port
    @param[in] handler pointer to an connection_handler.

    @param[in] socket_ the boost::asio::ip::tcp::socket on which the new connection was accepted.
  */
  tcp_connection(connection_id id, const std::string& address_, const boost::asio::ip::tcp::socket& socket_,
      std::shared_ptr<connection_handler> handler_);

  /**
    Destructor.
  */
  ~tcp_connection();

  /**
    @brief Initialises the connection.

    @pre tcp_init() to has been called before, error will happen, otherwise

    Parses the address and tries to create an endpoint. If it succeeds, connection state will be set to disconnected,
    error will be logged, otherwise.

    @returns true if the initialisation is successful, false if an error occures. If there was an error while
      initialisation, it will be logged.
  */
  bool init();


  /**
    Asynchronous connect. If an error occures, disconnect will be called and also handler's on_connection_error with
    status::unknown and a message containing the error. Status is changed to connecting. If a connection is successfully
    made, handler's on_connected will be called and the state is set to connected.
  */
  void connect();

  /**
    @see connection::async_send

    @pre tcp_initialized == true meaning tcp_init() to be called
    @pre socket to be open. Socket opens on connection construction or when connect() is called and didn't fail. If
      disconnect() has been called, connect() should be called before this function.
    @pre msg.size() > 0

    If any of the preconditions is not met on_message_sent() will be called with status, containing an error
    @see common::status

    If an error occures during sending, on_message_sent() will be called with status, containing the error. If the error
    is *broken pipe*, disconnect() will be called too.
  */
  void async_send(const std::string& msg, uint32_t id);

  /**
    @see connection::async_read

    @pre tcp_initialized == true meaning tcp_init() to be called
    @pre socket to be open. Socket opens on connection construction or when connect() is called and didn't fail. If
      disconnect() has been called, connect() should be called before this function.

    If any of the preconditions is not met on_connection_error() will be called with status::internal, containing an
    error. @see common::status

    If you call this function more than once, events form a queue, no read is cancelled.
    If an error occures during sending, on_connection_error() will be called with status, containing the error.
    If the error is boost::asio::error::eof (meaning the other peer has disconnected), disconnect() will be called too.
  */
  void async_read(std::shared_ptr<char> buffer, uint32_t buffer_size, uint32_t num_bytes, uint32_t id);

  /**
    This function can be called to disconnect peer. To reconnect, connect() should be called. Handler's
    on_disconnected() will be called on successful disconnect, on_connection_error(), otherwise.
  */
  void disconnect();

  /**
    This function is used to change the handler. It may be necessary when a connection is created from the acceptor
    and the default handler (the one passed to the acceptor) needs to be changed.
  */
  void add_handler(std::shared_ptr<connection_handler> handler_);

  std::string get_address() const;

  connection::state get_state() const;

 private:
  boost::asio::ip::tcp::endpoint asio_endpoint;
  boost::asio::ip::tcp::socket asio_socket;
  connection::state connection_state;
  std::string address;
  std::mutex connection_mutex;
  mutable std::mutex state_mutex;

  void set_state(connection::state new_state);
};

class tcp_acceptor:public acceptor, public std::enable_shared_from_this<tcp_acceptor> {
 public:
  /**
    @see acceptor::create
    @see acceptor::factory_function
  */
  tcp_acceptor(acceptor_id id, const std::string& address, std::shared_ptr<acceptor_handler> handler_,
      std::shared_ptr<connection::connection_handler> connections_handler);

  /**
    Destructor.
  */
  ~tcp_acceptor();


  /**
    @brief Initialises the acceptor.

    @pre tcp_init() to has been called, error will happen, otherwise

    Parses the address and tries to open an acceptor and bind it to the specified address. If port 0 is given in the
    address, random port will be assigned and the address of the acceptor will be updated to include the new port. If
    the initialisation is successful, the acceptor will start to listen for new connections, but none will be accepted
    until start_accepting is called.

    @returns true if the initialisation is successful, false if an error occures. If there was an error while
      initialisation, it will be logged.
  */
  bool init();

  // TODO(kari): Decide what to do on error.
  /**
    @brief The acceptor starts to listen for incoming connections. Asynchronous.

    This function is called from the constructor to start asynchronous accepting. When a remote peer request to make a
    connection, handler's on_requested() method will be called, passing peer's address and waiting for confirmaton to
    accept or not. If on_requested() returns true, then a connection is created and its handler's on_connected() called.
    Acceptor's handler's on_connected will also be called and a pointer to the created connection passed. If an error
    occured while accepting, handler's on_acceptor_error() will be called.

    @pre tcp_initialized == true meaning tcp_init() to be called
    @pre socket to be open. Socket opens on connection construction or when connect() is called and didn't fail. If
      disconnect() has been called, connect() should be called before this function.

    If any of the preconditions is not met on_acceptor_error() will be called with status::internal, containing an
    error. @see common::status
  */
  void start_accepting();

  void stop_accepting();

  std::string get_address() const;

  acceptor::state get_state() const;

 private:
  boost::asio::ip::tcp::acceptor asio_acceptor;
  std::shared_ptr<connection::connection_handler> accepted_connections_handler;
  acceptor::state acceptor_state;
  mutable std::mutex state_mutex;
  std::string address;

  void set_state(acceptor::state new_state);
};

/**
  Registers the connection and acceptor implementations with type "tcp".
  @see connection::register_connection_type
  @see acceptor::register_connection_type
  Initialising asio io_service and work objects. New thread is created and io_service.run() called in it.
  If a known exception happens while running the asio io_service, it will be logged.
*/
void tcp_init();

/**
  Stops the worker thread meaning all async operations will be cancelled. If this function is not called, segmentation
  fault will happen on program exit because of still running threads.
*/
void tcp_release();

void parse_address(const std::string&, std::string* result_addr, std::string* result_port);

}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_TCP_IMPLEMENTATION_H_
