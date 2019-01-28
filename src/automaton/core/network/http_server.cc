#include "automaton/core/network/http_server.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include "automaton/core/io/io.h"

using boost::asio::ip::tcp;

static const char* CRLF = "\r\n\r\n";

namespace automaton {
namespace core {
namespace network {

http_session::http_session(boost::asio::io_service& io_service, std::shared_ptr<http_server::server_handler> sh)
  : socket_(io_service),
    handler(sh)  {
      header = "";
      body = "";
}

tcp::socket& http_session::socket() {
  return socket_;
}

void http_session::start() {
  read_header();
}

void http_session::read_header() {
  uint32_t pos = header.find(CRLF);
  if (pos == -1) {
    socket_.async_read_some(boost::asio::buffer(buffer, kBufferSize),
        [this](const boost::system::error_code& error, size_t bytes_transferred) {
          if(!error) {
            header = header + std::string(buffer, bytes_transferred);
            read_header();
          } else if (error == boost::asio::error::eof) {
            LOG(ERROR) << "Client has closed the connection";
          } else {
            LOG(ERROR) << "Server error while reading body, deleting connection";
            delete this;
          }
    });
  } else if (pos + 4 < header.size()) {
    body = std::string(header, pos + 4);
    header.erase(pos + 4);
    read_header();
  } else {
    // parse_header
    std::stringstream ss(header);
    std::string line;
    uint32_t body_size = 0;
    while(std::getline(ss, line)) {
      if (line.substr(0, 16) == "Content-Length: ") {
        body_size = std::stoi(line.substr(16));
        break;
      }
    }
    header = "";
    if (body_size) {
      read_body(body_size);
    } else {
      read_header();
    }
  }
}

void http_session::read_body(uint32_t body_size) {
  uint32_t sz = body.size();
  if (sz < body_size) {
    boost::asio::async_read(socket_, boost::asio::buffer(buffer, kBufferSize), boost::asio::transfer_exactly(body_size - sz),
      [this, body_size](const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error) {
          body = body + std::string(buffer, bytes_transferred);
          read_body(body_size);
        } else if (error == boost::asio::error::eof) {
          LOG(ERROR) << "Client has closed the connection";
        } else {
          LOG(ERROR) << "Server error while reading body, deleting connection";
          delete this;
        }
    });
  } else if (sz > body_size) {
    header = std::string(body, body_size);
    body.erase(body_size);
    read_body(body_size);
  } else {
    std::string data = handler->handle(body);
    body = "";
    std::string response = add_http_header(data);
    boost::asio::async_write(socket_, boost::asio::buffer(response.c_str(), response.size()),
        [this](const boost::system::error_code& error, size_t bytes_transferred) {
          if (!error) {
            read_header();
          } else if (error == boost::asio::error::eof) {
            LOG(ERROR) << "Client has closed the connection";
          } else {
            LOG(ERROR) << "Server error while returning response to client, deleting connection";
            delete this;
          }
    });
  }
}

std::string http_session::add_http_header(const std::string& data) {
  std::stringstream ss;
  ss << "HTTP/1.0 200 OK\r\n";
  ss << "Content-Length: " << data.size() << "\r\n";
  ss << "Content-Type: text/plain\r\n";
  ss << "Connection: Closed\r\n";
  ss << "\r\n" << data;
  return ss.str();
}

http_server::http_server(uint16_t port, std::shared_ptr<server_handler> sh) :
    io_service(),
    acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
  LOG(INFO) << "Server constructor";
  handler = sh;
  http_session* new_session = new http_session(io_service, handler);
  acceptor.async_accept(new_session->socket(),
    boost::bind(&http_server::handle_accept, this, new_session,
    boost::asio::placeholders::error));
}

void http_server::handle_accept(http_session* new_session, const boost::system::error_code& error) {
  if (!error) {
    new_session->start();
    new_session = new http_session(io_service, handler);
    acceptor.async_accept(new_session->socket(),
      boost::bind(&http_server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
  } else {
    LOG(ERROR) << "Server error in handle_accept, deleting connection";
    delete new_session;
  }
}

void http_server::run() {
  LOG(DEBUG) << "server starting.";
  worker = new std::thread([this]() {
    try {
      io_service.run();
    }
    catch (std::exception& e) {
      LOG(ERROR) << "Could not run io_service in server";
    }
    LOG(DEBUG) << "server stopped.";
  });
}

void http_server::stop() {
  io_service.stop();
  worker->join();
}
}  // namespace network
}  // namespace core
}  // namespace automaton
