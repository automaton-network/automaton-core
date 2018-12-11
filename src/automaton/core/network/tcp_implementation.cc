#include "automaton/core/network/tcp_implementation.h"

#include <mutex>
#include <regex>
#include <sstream>
#include <thread>

#include <boost/asio/read.hpp>

#include "automaton/core/io/io.h"

using automaton::core::common::status;

namespace automaton {
namespace core {
namespace network {

// TODO(kari): proper exit, clear resources..
// TODO(kari): improve init function and handle the new thread
// TODO(kari): Add "if ! tcp_initialized" where necessary

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

static boost::asio::io_service asio_io_service;
static boost::asio::io_service::work work_(asio_io_service);
static std::thread* worker_thread;
static bool tcp_initialized = false;

// Connection functions

tcp_connection::tcp_connection(connection_id id, const std::string& address_, connection_handler* handler_):
    connection(id, handler_), asio_socket{asio_io_service},
    connection_state(connection::state::invalid_state), address(address_) {
  if (!tcp_initialized) {
    std::stringstream msg;
    msg << "TCP is not initialized! Call tcp_init() first!";
    LOG(ERROR) << address << " -> " <<  msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
}

// This is called only from acceptor
tcp_connection::tcp_connection(connection_id id, const std::string& addr,
    const boost::asio::ip::tcp::socket& sock,
    connection_handler* handler_):connection(id, handler_),
    asio_socket(std::move(const_cast<boost::asio::ip::tcp::socket&>(sock))),
    connection_state(connection::state::connected), address(addr) {
}

tcp_connection::~tcp_connection() {
  // LOG(DEBUG) << "Connection destructor";
  connection_mutex.lock();
  set_state(connection::state::disconnected);
  if (asio_socket.is_open()) {
    boost::system::error_code boost_error_code_shut;
    asio_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, boost_error_code_shut);
    if (boost_error_code_shut) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_shut.message();
    }
    boost::system::error_code boost_error_code_close;
    asio_socket.close(boost_error_code_close);
    if (boost_error_code_close) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_close.message();
    }
    connection_mutex.unlock();
  } else {
    connection_mutex.unlock();
  }
  boost::system::error_code boost_error_code;
  asio_socket.release(boost_error_code);
  if (boost_error_code) {
    // LOG(DEBUG) << address << " -> " <<  boost_error_code.message();
  }
  // LOG(DEBUG) << "/ Connection destructor";
}

bool tcp_connection::init() {
  if (tcp_initialized) {
    try {
      boost::system::error_code boost_error_code;
      boost::asio::ip::tcp::resolver resolver{asio_io_service};
      std::string ip, port;
      parse_address(address, &ip, &port);
      boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(ip, port, boost_error_code);
      if (boost_error_code) {
        LOG(ERROR) << address << " -> " <<  boost_error_code.message();
        return false;
      } else {
        asio_endpoint = *it;
        set_state(connection::state::disconnected);
        return true;
      }
    } catch (...) {
      LOG(ERROR) << address << " -> " << "Exception";
      return false;
    }
  } else {
    LOG(ERROR) << address << " -> " <<  "Not initialized! tcp_init() must be called first";
    return false;
  }
}

void tcp_connection::connect() {
  if (tcp_initialized) {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (get_state() == connection::state::disconnected) {
      set_state(connection::state::connecting);
      std::shared_ptr<tcp_connection> self = shared_from_this();
      connection_handler* c_handler = handler;
      connection_id cid = id;
      std::string addr = address;
      asio_socket.async_connect(asio_endpoint,
          [self, c_handler, cid, addr](const boost::system::error_code& boost_error_code) {
            if (boost_error_code) {
              self->disconnect();
              // set_state(connection::state::disconnected);
              LOG(ERROR) << addr << " -> " <<  boost_error_code.message();
              c_handler->on_connection_error(cid, status::unknown(boost_error_code.message()));
            } else {
              self->set_state(connection::state::connected);
              c_handler->on_connected(cid);
            }
      });
    }
  } else {
    std::stringstream s;
    s << address << " -> " <<  "Not initialized! tcp_init() must be called first";
    LOG(ERROR) << s.str();
    handler->on_connection_error(id, status::unknown(s.str()));
    // TODO(kari): what to do here? needs to be connected
  }
}

void tcp_connection::async_send(const std::string& msg, uint32_t message_id) {
  if (tcp_initialized && asio_socket.is_open() && msg.size() > 0) {
    // LOG(DEBUG) << "ASYNC SEND MSG ID" << message_id << " data: " << io::bin2hex(msg);
    std::string* message = new std::string(msg);
    std::shared_ptr<tcp_connection> self = shared_from_this();
    connection_handler* c_handler = handler;
    connection_id cid = id;
    std::string addr = address;
    asio_socket.async_write_some(boost::asio::buffer(*message),
        [self, c_handler, cid, addr, message_id, message](const boost::system::error_code& boost_error_code,
        size_t bytes_transferred) {
      // LOG(DEBUG) << "ASYNC SEND CALLBACK " << bytes_transferred;
      if (boost_error_code) {
        LOG(ERROR) << addr << " -> " <<  boost_error_code.message();
        if (boost_error_code == boost::asio::error::broken_pipe) {
          c_handler->on_message_sent(cid, message_id, status::aborted(boost_error_code.message()));
          // TODO(kari): ?? handle
          self->disconnect();
        } else if (boost_error_code == boost::asio::error::operation_aborted) {
          c_handler->on_message_sent(cid, message_id, status::aborted("Operation cancelled!"));
        } else {
          c_handler->on_message_sent(cid, message_id, status::unknown(boost_error_code.message()));
        }
       // if (bytes_transferred < message.size())
      } else {
        // LOG(DEBUG)
        //     << "SUCCESSFULLY SENT MESSAGE WITH "
        //     << message->size() << " BYTES TO " << id << " msg_id " << message_id;
        c_handler->on_message_sent(cid, message_id, status::ok());
      }
      delete message;
    });
  } else if (!tcp_initialized) {
    LOG(ERROR) << address << " -> " <<  "Not initialized";
    handler->on_message_sent(id, message_id, status::internal("Not initialized"));
    // TODO(kari): what to do here? needs to be connected
  } else if (msg.size() <= 0) {
    LOG(ERROR) << address << " -> " <<  "Message too short";
    handler->on_message_sent(id, message_id, status::invalid_argument("Message too short"));
    // TODO(kari): what to do here? needs to be connected
  } else {
    LOG(ERROR) << address << " -> " <<  "Socket closed or not yet connected";
    handler->on_message_sent(id, message_id, status::internal("Socket closed or not yet connected"));
  }
}

void tcp_connection::async_read(char* buffer, uint32_t buffer_size,
    uint32_t num_bytes, uint32_t read_id) {
  if (tcp_initialized && asio_socket.is_open()) {
    if (num_bytes == 0) {
      std::shared_ptr<tcp_connection> self = shared_from_this();
      connection_handler* c_handler = handler;
      connection_id cid = id;
      std::string addr = address;
      asio_socket.async_read_some(boost::asio::buffer(buffer, buffer_size),
          [self, c_handler, cid, addr, buffer, read_id](const boost::system::error_code& boost_error_code,
          size_t bytes_transferred) {
        if (boost_error_code) {
          if (boost_error_code == boost::asio::error::eof) {
            LOG(ERROR) << addr << " -> " <<  "Peer has closed the connection";
            c_handler->on_connection_error(cid, status::aborted("Peer has closed the connection"));
            self->disconnect();
            return;
          } else if (boost_error_code == boost::asio::error::operation_aborted) {
            c_handler->on_connection_error(cid, status::aborted("Operation cancelled!"));
            return;
          } else {
            LOG(ERROR) << addr << " -> " <<  boost_error_code.message();
            c_handler->on_connection_error(cid, status::unknown(boost_error_code.message()));
            // TODO(kari): what errors and when should read be called?
          }
        } else {
          c_handler->on_message_received(cid, buffer, bytes_transferred, read_id);
        }
      });
    } else {
        std::shared_ptr<tcp_connection> self = shared_from_this();
        connection_handler* c_handler = handler;
        connection_id cid = id;
        std::string addr = address;
        boost::asio::async_read(asio_socket, boost::asio::buffer(buffer, buffer_size),
          boost::asio::transfer_exactly(num_bytes),
          [self, c_handler, addr, cid, buffer, read_id](const boost::system::error_code& boost_error_code,
              size_t bytes_transferred) {
        if (boost_error_code) {
          if (boost_error_code == boost::asio::error::eof) {
            self->disconnect();
            return;
          } else if (boost_error_code == boost::asio::error::operation_aborted) {
            return;
          } else {
            LOG(ERROR) << addr << " -> " <<  boost_error_code.message();
            c_handler->on_connection_error(cid, status::unknown(boost_error_code.message()));
            // TODO(kari): what errors and when should read be called?
          }
        } else {
          c_handler->on_message_received(cid, buffer, bytes_transferred, read_id);
        }
      });
    }
  } else if (!tcp_initialized) {
    LOG(ERROR) << address << " -> " <<  "Not initialized";
    handler->on_connection_error(id, status::internal("Not initialized"));
    // TODO(kari): what to do here? needs to be connected
  } else {
    LOG(ERROR) << address << " -> " <<  "Socket closed";
    handler->on_connection_error(id, status::internal("Socket closed"));
  }
}

void tcp_connection::disconnect() {
  connection_mutex.lock();
  set_state(connection::state::disconnected);
  if (asio_socket.is_open()) {
    boost::system::error_code boost_error_code_shut;
    asio_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, boost_error_code_shut);
    if (boost_error_code_shut) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_shut.message();
    }
    boost::system::error_code boost_error_code_close;
    asio_socket.close(boost_error_code_close);
    if (boost_error_code_close) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_close.message();
    }
    connection_mutex.unlock();
    handler->on_disconnected(id);
  } else {
    connection_mutex.unlock();
  }
}

void tcp_connection::add_handler(connection_handler* handler_) {
  std::lock_guard<std::mutex> lock(connection_mutex);
  this->handler = handler_;
}

std::string tcp_connection::get_address() const {
  return address;
}

connection::state tcp_connection::get_state() const {
  std::lock_guard<std::mutex> lock(state_mutex);
  return connection_state;
}

void tcp_connection::set_state(connection::state new_state) {
  std::lock_guard<std::mutex> lock(state_mutex);
  connection_state = new_state;
}

// Acceptor functions

tcp_acceptor::tcp_acceptor(acceptor_id id, const std::string& address, acceptor_handler* handler,
    connection::connection_handler* connections_handler_):
    acceptor(id, handler), asio_acceptor{asio_io_service}, accepted_connections_handler(connections_handler_),
    acceptor_state(acceptor::state::invalid_state), address(address) {
  if (!tcp_initialized) {
    std::stringstream msg;
    msg << "TCP is not initialized! Call tcp_init() first!";
    LOG(ERROR) << address << " -> " <<  msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
}

tcp_acceptor::~tcp_acceptor() {
  // LOG(DEBUG) << "Acceptor destructor";
  if (asio_acceptor.is_open()) {
    boost::system::error_code boost_error_code_close;
    asio_acceptor.close(boost_error_code_close);
    if (boost_error_code_close) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_close.message();
    }
    boost::system::error_code boost_error_code_release;
    asio_acceptor.release(boost_error_code_release);
    if (boost_error_code_release) {
      // LOG(DEBUG) << address << " -> " <<  boost_error_code_close.message();
    }
  }
  // LOG(DEBUG) << "/ Acceptor destructor";
}

bool tcp_acceptor::init() {
  if (tcp_initialized) {
    boost::asio::ip::tcp::resolver resolver{asio_io_service};
    boost::system::error_code boost_error_code;
    std::string ip, port;
    parse_address(address, &ip, &port);
    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(ip, port, boost_error_code);
    if (boost_error_code) {
      LOG(ERROR) << address << " -> " <<  boost_error_code.message();
      return false;
    } else {
      boost::system::error_code ecc;
      asio_acceptor.open(((boost::asio::ip::tcp::endpoint)*it).protocol());
      asio_acceptor.bind(*it, ecc);
      if (ecc) {
        LOG(ERROR) << address << " -> " <<  ecc.message();
        return false;
      } else {
        std::stringstream s;
        s << asio_acceptor.local_endpoint().address() << ':' << asio_acceptor.local_endpoint().port();
        address = s.str();
        asio_acceptor.listen();
        return true;
      }
    }
  } else {
    LOG(ERROR) << address << " -> " <<  "Not initialized! tcp_init() must be called first";
    return false;
  }
}

void tcp_acceptor::start_accepting() {
  if (tcp_initialized && asio_acceptor.is_open()) {
    set_state(acceptor::state::accepting);
    std::shared_ptr<tcp_acceptor> self = shared_from_this();
    acceptor_handler* a_handler = handler;
    connection::connection_handler* ac_handler = accepted_connections_handler;
    asio_acceptor.async_accept(asio_io_service, [self, a_handler, ac_handler]
        (const boost::system::error_code& boost_error_code, boost::asio::ip::tcp::socket socket_) {
       // LOG(DEBUG) << "async_accept";
       if (!boost_error_code) {
         // LOG(DEBUG) << "async_accept -> no error";
          boost::asio::ip::tcp::endpoint remote_endpoint = socket_.remote_endpoint();
          std::string remote_address = (remote_endpoint.address()).to_string() +
              ":" + std::to_string(remote_endpoint.port());
          connection_id id = 0;
          // LOG(DEBUG) << "async_accept calling on_requested conn";
          bool accepted = a_handler->on_requested(self->get_id(), remote_address, &id);
          if (accepted) {
            // LOG(DEBUG) << "async_accept ACCEPTED";
            std::shared_ptr<connection> new_con(new tcp_connection(id, remote_address, socket_, ac_handler));
            // LOG(DEBUG) << "async_accept calling on_connected acc";
            a_handler->on_connected(self->get_id(), std::move(new_con), remote_address);
            if (ac_handler) {
              // LOG(DEBUG) << "async_accept calling on_connected conn";
              ac_handler->on_connected(id);
            }
          }
          // LOG(DEBUG) << "async_accept calling start_accepting";
          self->start_accepting();
        } else {
          // LOG(DEBUG) << "async_accept -> error";
          self->set_state(acceptor::state::not_accepting);
          // TODO(kari): Handle errors
          if (boost_error_code == boost::asio::error::operation_aborted) {
            // LOG(DEBUG) << "end of async_accept 0";
            return;
          }
          LOG(ERROR) << boost_error_code.message();
          // LOG(DEBUG) << "async_accept calling on_error";
          a_handler->on_acceptor_error(self->get_id(), status::unknown(boost_error_code.message()));
          // TODO(kari): start listen again? depends on the errors
          // start_accepting();
        }
        // LOG(DEBUG) << "end of async_accept";
      });
  } else if (!tcp_initialized) {
    LOG(ERROR) <<  "Not initialized";
    handler->on_acceptor_error(id, status::internal("Not initialized"));
    // TODO(kari): what to do here? needs to be connected
  } else {
    LOG(ERROR) <<  "Acceptor closed";
    handler->on_acceptor_error(id, status::internal("Acceptor closed!"));
  }
}

std::string tcp_acceptor::get_address() const {
  return address;
}

acceptor::state tcp_acceptor::get_state() const {
  // TODO(kari): implement this
  return acceptor_state;
}

void tcp_acceptor::set_state(acceptor::state new_state) {
  std::lock_guard<std::mutex> lock(state_mutex);
  acceptor_state = new_state;
}

// Global functions

void tcp_init() {
  if (tcp_initialized) {
    return;
  }
  connection::register_connection_type("tcp",
    [](connection_id id, const std::string& address, connection::connection_handler* handler) ->
        std::shared_ptr<connection> {
      return std::shared_ptr<connection>(new tcp_connection(id, address, handler));
    });
  acceptor::register_acceptor_type("tcp",
    [](acceptor_id id, const std::string& address, acceptor::acceptor_handler* handler,
    connection::connection_handler* connections_handler) -> std::shared_ptr<acceptor> {
      return std::shared_ptr<acceptor>(new tcp_acceptor(id, address, handler, connections_handler));
    });
  worker_thread = new std::thread([]() {
    LOG(DEBUG) << "asio_io_service starting...";
    try {
      asio_io_service.run();
    } catch (const std::exception& ex) {
      LOG(ERROR) << el::base::debug::StackTrace();
      LOG(FATAL) << "ASIO THREAD EXCEPTION: " << ex.what();
    } catch (...) {
      LOG(FATAL) << "EXCEPTION!!!!";
    }
    LOG(DEBUG) << "asio_io_service stopped.";
  });
  tcp_initialized = true;
}

void tcp_release() {
  LOG(DEBUG) << "Stopping io_service";
  asio_io_service.stop();
  LOG(DEBUG) << "joining worker_thread..";
  worker_thread->join();
  LOG(DEBUG) << "tcp_release done.";
  tcp_initialized = false;
}

void parse_address(const std::string& address, std::string* result_addr, std::string* result_port) {
  std::regex rgx_ip("((?:\\d+\\.)+\\d+|(?:[0-9a-f]+:)+[0-9a-f]+):(\\d+)");
  std::smatch match;
  if (std::regex_match(address.begin(), address.end(), match, rgx_ip) &&
      match.size() == 3) {
    *result_addr = match[1];
    *result_port = match[2];
  } else {
    *result_addr = "";
    *result_port = "";
  }
}

}  // namespace network
}  // namespace core
}  // namespace automaton
