#include "automaton/core/network/rpc.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <boost/asio/basic_stream_socket.hpp>  // NOLINT
#include <boost/asio/buffer.hpp>  // NOLINT
#include <boost/asio/error.hpp>  // NOLINT
#include <boost/asio/io_service.hpp>  // NOLINT
#include <boost/asio/ip/tcp.hpp>  // NOLINT
#include <boost/asio/placeholders.hpp>  // NOLINT
#include <boost/asio/write.hpp>  // NOLINT
#include <boost/bind.hpp>  // NOLINT
#include "automaton/core/io/io.h"  // NOLINT

using boost::asio::ip::tcp;


namespace automaton {
namespace core {
namespace network {


session::session(boost::asio::io_service& io_service) // NOLINT
  : socket_(io_service) {
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
    std::cout << std::string(data_, bytes_transferred) << std::endl;
    boost::asio::async_write(socket_,
      boost::asio::buffer(data_, bytes_transferred),
      boost::bind(&session::handle_write, this,
        boost::asio::placeholders::error));
  } else {
    LOG(ERROR) << "Error in handle_read, deleting connection";
    delete this;
  }
}

void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    socket_.async_read_some(boost::asio::buffer(data_, kBufferSize),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  } else {
    LOG(ERROR) << "Error in handle_write, deleting connection";
    delete this;
  }
}


server::server(uint16_t port, std::string(*func)(std::string))
  : acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
    handler(handler) {
  session* new_session = new session(io_service);
  acceptor.async_accept(new_session->socket(),
    boost::bind(&server::handle_accept, this, new_session,
      boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error) {
  if (!error) {
    new_session->start();
    new_session = new session(io_service);
    acceptor.async_accept(new_session->socket(),
      boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
  } else {
    LOG(ERROR) << "Error in handle_accept, deleting connection";
    delete new_session;
  }
}

void server::run() {
  worker = new std::thread([this]() {
    try {
      io_service.run();
    }
    catch (std::exception& e) {
      LOG(ERROR) << "Could not run io_service";
      throw std::exception("Could not run io_service");
    }
    LOG(DEBUG) << "asio_io_service stopped.";
  });
}

void server::stop() {
  io_service.stop();
  worker->join();
}

}  // namespace network
}  // namespace core
}  // namespace automaton
