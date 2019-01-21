#include "automaton/core/network/rpc_server.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include "automaton/core/io/io.h"

using boost::asio::ip::tcp;

namespace automaton {
namespace core {
namespace network {

session::session(boost::asio::io_service& io_service, std::shared_ptr<server::server_handler> sh)
  : socket_(io_service),
    handler(sh)  {
}

tcp::socket& session::socket() {
  return socket_;
}

void session::start() {
  socket_.async_read_some(boost::asio::buffer(data_, kBufferSize),
    boost::bind(&session::handle_read, this,
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
  if (!error) {
    std::string response = handler->handle(std::string(data_, bytes_transferred));
    boost::asio::async_write(socket_,
      boost::asio::buffer(response.c_str(), response.size()),
      boost::bind(&session::handle_write, this,
        boost::asio::placeholders::error));
  } else if (error == boost::asio::error::eof) {
    LOG(ERROR) << "Peer has closed the connection";
  } else {
    LOG(ERROR) << "Server error in handle_read, deleting connection";
    delete this;
  }
}

void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    socket_.async_read_some(boost::asio::buffer(data_, kBufferSize),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  } else if (error == boost::asio::error::eof) {
    LOG(ERROR) << "Peer has closed the connection";
  } else {
    LOG(ERROR) << "Server error in handle_write, deleting connection";
    delete this;
  }
}

server::server(uint16_t port, std::shared_ptr<server_handler> sh) :
    io_service(),
    acceptor(io_service, tcp::endpoint(tcp::v4(), port))
  {
  LOG(INFO) << "Server constructor";
  handler = sh;
  session* new_session = new session(io_service, handler);
  acceptor.async_accept(new_session->socket(),
    boost::bind(&server::handle_accept, this, new_session,
    boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error) {
  if (!error) {
    new_session->start();
    new_session = new session(io_service, handler);
    acceptor.async_accept(new_session->socket(),
      boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
  } else {
    LOG(ERROR) << "Server error in handle_accept, deleting connection";
    delete new_session;
  }
}

void server::run() {
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

void server::stop() {
  io_service.stop();
  worker->join();
}
}  // namespace network
}  // namespace core
}  // namespace automaton
