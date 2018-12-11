#ifndef AUTOMATON_CORE_NETWORK_ACCEPTOR_H_
#define AUTOMATON_CORE_NETWORK_ACCEPTOR_H_

#include <map>
#include <memory>
#include <string>

#include "automaton/core/common/status.h"
#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace network {

typedef uint32_t acceptor_id;

// Class that is used to listen for and accept incoming connections. (Server)
class acceptor {
 public:
  enum state {
    invalid_state = 0,
    accepting = 1,
    not_accepting = 2,
  };
  /**
    Handler class used to inform the client for events.
      - on_requested will be invoked when a peer wants to connect; it passes the address of the remote peer.
      - on_connected will be invoked if the connection was accepted by the user and new connection was made; it passes a
        shared pointer of the resulting connection
      - on_error will be invoked when an error happens while listening or accepting.
  */
  class acceptor_handler {
   public:
    virtual ~acceptor_handler() {}
    // IDEA(kari): return string (schema message with connection params) instead of bool

    /**
      Will be invoked when a peer sends a connection request. If this function returns true, new connection will be
      created and passed later with on_connected or on_acceptor_error will be called if an error occures.

      @param[in] a the id of the acceptor that caused the event
      @param[in] address the address of the peer requesting the connection
      @param[out] id the id to be assigned to the newly created connection if the function returns true. If it returns
        false, id is ignored

      @return true if the user want to accept a connection from this address, false otherwise
    */
    virtual bool on_requested(acceptor_id a, const std::string& address, connection_id* id) = 0;

    /**
      Will be called when a connection is accepted, passing the newly created connection.

      @param[in] a the id of the acceptor that accepted the connection
      @param[in] c the newly created connection
      @param[in] address the address of the accepted connection as it was when on_requested was invoked
    */
    virtual void on_connected(acceptor_id a, std::shared_ptr<connection> c, const std::string& address) = 0;

    /**
      Will be invoked when an error occurred while listening for or accepting connection to notify the user about the
      event.

      @param[in] a the id of the acceptor
      @param[in] s status containing the error type and a description
        @see common::status
    */
    virtual void on_acceptor_error(acceptor_id a, const common::status& s) = 0;
  };
  virtual ~acceptor() {}

  /**
    Initialises the acceptor.

    @return true if no errors occured and initialisation was successful, false, otherwise
  */
  virtual bool init() = 0;

  /**
    The acceptor starts to listen for incoming connections.
  */
  virtual void start_accepting() = 0;

  /**
    @returns the acceptor's address
  */
  virtual std::string get_address() const = 0;

  /**
    @returns the acceptor's state
  */
  virtual acceptor::state get_state() const = 0;

  /**
    @returns the acceptor's id
  */
  acceptor_id get_id();

  /**
    Function that is used to create objects from a specified child class.

    @param[in] type string representing the connection type/ child class ("sim", "tcp", etc.). The child class should
      first be registered using register_acceptor_type function.
      @see register_acceptor_type.
    @param[in] id identifier given by the user. It is used in acceptor_handler's functions to specify the acceptor
      invoked the function
      @see acceptor_handler
    @param[in] address string representing the address of the acceptor. Get_address does **NOT** necesseraly returns
      this. See child classes documentation for more info
    @param[in] handler_ pointer to an acceptor_handler
    @param[in] connections_handler_ pointer to a connection_handler. It will be used when accepting new connection to
      create a connection object
      @see connection
      @see acceptor::acceptor_handler

    @returns object from the specified class. If no such class type was registered, empty ptr will be returned.
  */
  static std::shared_ptr<acceptor> create(const std::string& type, acceptor_id id, const std::string& address,
      acceptor_handler* handler_, connection::connection_handler* connections_handler);

  typedef std::shared_ptr<acceptor> (*factory_function)(acceptor_id id, const std::string& address,
      acceptor_handler* handler_, connection::connection_handler* connections_handler);

  /**
    Registers acceptor implementation.

    @param[in] type shows how the class will be referenced (e.g. "tcp"). If such type name exists in the
    registry, the factory_function pointer will be overriden
    @param[in] factory_function specifies the function tht will be used to create the child object
      @see factory_function
  */
  static void register_acceptor_type(const std::string& type, factory_function func);

 protected:
  /**
    Class constructor.

    @param[in] id identifier given by the user. It is used in acceptor_handler's functions to specify the acceptor
      invoked the function
      @see acceptor_handler
    @param[in] handler_ pointer to an acceptor_handler
  */
  acceptor(acceptor_id id, acceptor_handler* handler_);

  acceptor_handler* handler;
  acceptor_id id;

 private:
  /**
    Map that stores registered funtions that are used to create child objects.
  */
  static std::map<std::string, factory_function> acceptor_factory;
};

}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_ACCEPTOR_H_
