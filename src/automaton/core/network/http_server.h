#ifndef AUTOMATON_CORE_NETWORK_HTTP_SERVER_H_
#define AUTOMATON_CORE_NETWORK_HTTP_SERVER_H_

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <boost/config/warning_disable.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "automaton/core/io/io.h"

namespace automaton {
namespace core {
namespace network {

class http_session;

class http_server {
 public:
  enum status_code {
    OK = 0,
    NO_CONTENT = 1,
    BAD_REQUEST = 2,
    UNAUTHORIZED = 3,
    FORBIDDEN = 4,
    INTERNAL_SERVER_ERROR = 5,
    NOT_IMPLEMENTED = 6,
    SERVICE_UNAVAILABLE = 7,
  };

  static const char* ok;
  static const char* no_content;
  static const char* bad_request;
  static const char* unauthorized;
  static const char* forbidden;
  static const char* internal_server_error;
  static const char* not_implemented;
  static const char* service_unavailable;

  static const std::map<uint32_t, const char*> status_to_string;

  class server_handler {
   public:
    server_handler() {}
    virtual ~server_handler() {}
    virtual std::string handle(std::string, status_code*) = 0;
  };
  http_server(uint16_t port, std::shared_ptr<server_handler>);
  void handle_accept(http_session* new_session, const boost::system::error_code& error);
  void run();
  void stop();

 private:
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::acceptor acceptor;
  std::shared_ptr<server_handler> handler;
  std::thread* worker;
};

class http_session {
 public:
  http_session(boost::asio::io_service& io_service, std::shared_ptr<http_server::server_handler>);  // NOLINT

  boost::asio::ip::tcp::socket& socket();

  void start();

  void read_header();

  void read_body(uint32_t body_size);

  std::string add_http_header(const std::string& body, http_server::status_code s) const;

 private:
  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<http_server::server_handler> handler;
  static const size_t kBufferSize = 1024;
  char buffer[kBufferSize];
  std::string header;
  std::string body;
};
}  // namespace network
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NETWORK_HTTP_SERVER_H_
