#ifndef AUTOMATON_CORE_NETWORK_RPC_SERVER_H_
#define AUTOMATON_CORE_NETWORK_RPC_SERVER_H_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "automaton/core/io/io.h"

namespace automaton {
namespace core {
namespace network {

class session;

class server {
 public:
  class server_handler {
   public:
    server_handler() {}
    ~server_handler() {}
    virtual std::string handle(std::string) = 0;
  };
  server(uint16_t port, std::shared_ptr<server_handler>);
  void handle_accept(session* new_session, const boost::system::error_code& error);
  void run();
  void stop();

 private:
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::acceptor acceptor;
  std::shared_ptr<server_handler> handler;
  std::thread* worker;
};

class session {
 public:
  session(boost::asio::io_service& io_service, std::shared_ptr<server::server_handler>);  // NOLINT

  boost::asio::ip::tcp::socket& socket();

  void start();

  void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

  void handle_write(const boost::system::error_code& error);

 private:
  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<server::server_handler> handler;
  static const size_t kBufferSize = 1024;
  char data_[kBufferSize];
};
}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_RPC_SERVER_H_
