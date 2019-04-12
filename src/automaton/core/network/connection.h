#ifndef AUTOMATON_CORE_NETWORK_CONNECTION_H_
#define AUTOMATON_CORE_NETWORK_CONNECTION_H_

#include <map>
#include <memory>
#include <string>
#include <thread>

#include "automaton/core/common/status.h"

namespace automaton {
namespace core {
namespace network {

// TODO(kari): think about `send and disconnect`

typedef uint32_t connection_id;

/**
  Class that represents a connection between two peers. It is used to connect to remote peer or is used by the acceptor
  class when accepts an incoming connection.
*/

class connection {
 public:
  /**
    TODO(kari) documentation
  */
  enum state {
    invalid_state = 0,
    connecting = 1,
    connected = 2,
    disconnected = 3,
  };

  /**
    Handler class used to inform the client for events.
      - on_message_received will be invoked when message from remote peer was
        received; it passes the connection, representing from which peer was the
        message, and the message that was received.
      - on_message_sent will be invoked when a message was sent successfully
        using async_send.
      - on_connected will be invoked when the connection with the remote peer
        was established.
      - on_disconnected will be invoked when connection was closed/ destroyed
        by the local or the remote peer
      - on_error will be invoked when an error happens while listening or
        accepting.
  */
  class connection_handler {
   public:
    virtual ~connection_handler() {}

    /**
      Will be invoked to notify the user about a received message

      @param[in] c the id of the connection where the message was received
      @param[in] buffer the buffer where the message is stored
      @param[in] bytes_read the size of the received message
      @param[in] id the identifier of the read operation
        @see connection::async_read
    */
    virtual void on_message_received(connection_id c, std::shared_ptr<char> buffer, uint32_t bytes_read,
        uint32_t id) = 0;

    /**
      Will be invoked to notify the user that a message was successfully sent or an error occured.

      @param[in] c the id of the connection
      @param[in] id the id of the message/send operation
        @see connection::async_send
      @param[in] s the status of the operation. If it was not successful s will contain the error and a description,
        s = status::ok, otherwise
        @see common::status
    */
    virtual void on_message_sent(connection_id c, uint32_t id, const common::status& s) = 0;

    /**
      Will be invoked to inform the user that a connection was successfully made after connect was called.

      @param[in] c the connection id
    */
    virtual void on_connected(connection_id c) = 0;

    /**
      Will be invoked to inform the user that a disconnection was successfully made.

      @param[in] c the connection id
    */
    virtual void on_disconnected(connection_id c) = 0;

    /**
      Will be invoked when error occures in a connection to notify the user about the event.

      @param[in] c the id of the connection
      @param[in] s status containing the error type and a description
        @see common::status
    */
    virtual void on_connection_error(connection_id c, const common::status& s) = 0;
  };
  virtual ~connection() {}

  /**
    Initialises the connection.

    @return true if no errors occured and initialisation was successful, false, otherwise
  */
  virtual bool init() = 0;

  /**
    Function that is used to send message to the remote peer. On_message_sent will(depending on the specific
    implementation) be invoked once the message was sent successfully

    @param[in] message the message to be sent
    @param[in] id given by the user identifying this concrete message. It will be used when on_message_sent is
      called to inform the user about successfully sent message
      @see connection_handler::on_message_sent
  */
  virtual void async_send(const std::string& message, uint32_t id = 0) = 0;

  /**
    Function that is used to read incoming messages. Messages could be received but not read if this function was not
    called. When a message is ready to be read, handler's on_message_received will be called. If an error occures,
    handler's on_connection_error will be called.
    @see connection_handler

    @param[in] buffer received message with size max(buffer_size, num_bytes) will be stored here
    @param[in] buffer_size the size of the buffer
    @param[in] num_bytes the number of bytes that are required to be read. If a message with greater size is received,
      only num_bytes will be read. If a message with smaller size is received, it will **not** be read until at least
      num_bytes are received. If num_bytes = 0 then a message will be read when it is received no matter the
      size (but max buffer_size)
    @param[in] id given by the user to identify this specific read call. It will be used from the handler when
      on_message_received is called. @see connection_handler::on_message_received
  */
  virtual void async_read(std::shared_ptr<char> buffer, uint32_t buffer_size, uint32_t num_bytes = 0,
      uint32_t id = 0) = 0;

  /**
    @return the connection id
    @see connection::create
  */
  connection_id get_id();

  /**
    @return the connection state
  */
  virtual state get_state() const = 0;

  /**
    @return the connection address
  */
  virtual std::string get_address() const = 0;

  /**
    Asynchronous connect. If an error occures, handler's on_connection_error will be called (depends on the specific
    implementation). If a connection is successfully made, handler's on_connected will be called (depends on the
    specific implementation).
  */
  virtual void connect() = 0;

  /**
    Disconnects with the remote peer. Handler's on_disconnected is called.
  */
  virtual void disconnect() = 0;

  /**
    Function that is used to create objects from a specified child class.

    @param[in] type string representing the connection type/ child class ("sim", "tcp", etc.). The child class should
      first be registered using register_connection_type function
      @see register_connection_type
    @param[in] id identifier given by the user. It is used in connection_handler's functions to specify the connection
      invoked the function. @see connection_handler
    @param[in] address string representing the address of the connection. Get_address does **NOT** necesseraly returns
      this. See child classes documentation for more info
    @param[in] handler pointer to an connection_handler

    @returns object from the specified class. If no such class type was registered, empty ptr will be returned.

  */
  static std::shared_ptr<connection> create(const std::string& type, connection_id id, const std::string& address,
      std::shared_ptr<connection_handler> handler);

  typedef std::shared_ptr<connection>
      (*factory_function)(connection_id id, const std::string& address, std::shared_ptr<connection_handler> handler);

  /**
    Registers connection implementation.

    @param[in] type shows how the class will be referenced (e.g. "tcp"). If such type name exists in the
    registry, the factory_function pointer will be overriden
    @param[in] factory_function specifies the function that will be used to create the child object
      @see factory_function
  */
  static void register_connection_type(const std::string& type, factory_function func);

 protected:
  /**
  Class constructor.

  @param[in] id identifier given by the user. It is used in connection_handler's functions to specify the connection
    invoked the function. @see connection_handler
  @param[in] handler_ pointer to an connection_handler
  */
  connection(connection_id id, std::shared_ptr<connection_handler> handler_);

  /**
    Handler object that must be set so the client could be informed for events.
    If no handler or a handler with empty function implementations is provided,
    client will not have access to events information like connect/ disconnect,
    received messages or an error that happend.
  */
  std::shared_ptr<connection_handler> handler;
  connection_id id;

 private:
  /**
    Map that stores registered child classes and pointers to funtions that are
    used to create objects.
  */
  static std::map<std::string, factory_function> connection_factory;
};

}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_CONNECTION_H_
