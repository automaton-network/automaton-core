#include "automaton/core/network/rpc.h"
#include <cstdlib>
#include <thread>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "automaton/core/io/io.h"

using boost::asio::ip::tcp;


namespace automaton {
namespace core {
namespace network {


class session {
 public:
  session(boost::asio::io_service& io_service)
    : socket_(io_service) {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start() {
    socket_.async_read_some(boost::asio::buffer(data_, buffer_size),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }

  void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
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

  void handle_write(const boost::system::error_code& error) {
    if (!error) {
      socket_.async_read_some(boost::asio::buffer(data_, buffer_size),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    } else {
      LOG(ERROR) << "Error in handle_write, deleting connection";
      delete this;
    }
  }
 private:
  tcp::socket socket_;
  const static uint32_t buffer_size = 1024;
  char data_[buffer_size];
};


class server {
 public:
  server(uint16_t port, std::string(*func)(std::string))
    : acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      handler(handler) {
    session* new_session = new session(io_service);
    acceptor.async_accept(new_session->socket(),
      boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session, const boost::system::error_code& error) {
    if (!error) {
      new_session->start();
      new_session = new session(io_service);
      acceptor.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
    }
    else {
      LOG(ERROR) << "Error in handle_accept, deleting connection";
      delete new_session;
    }
  }

  void run() {
    worker = new std::thread([this]() {
      try {
        io_service.run();
      }
      catch (std::exception& e)
      {
        LOG(ERROR) << "Could not run io_service";
        throw std::exception("Could not run io_service");
      }
      LOG(DEBUG) << "asio_io_service stopped.";
    });
  }
  void stop() {
    io_service.stop();
    worker->join();
  }

 private:
  std::string (*handler)(std::string json_str);
  boost::asio::io_service io_service;
  tcp::acceptor acceptor;
  std::thread* worker;
  std::atomic_bool done;
};

}  // namespace network
}  // namespace core
}  // namespace automaton