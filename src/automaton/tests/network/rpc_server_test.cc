#include "automaton/core/network/rpc_server.h"
#include <memory>
#include <string>
#include "gtest/gtest.h"
#include "automaton/core/network/tcp_implementation.h"

static const uint32_t PORT = 33445;
static const char* SERVER_ADDRESS = "127.0.0.1:33445";
using namespace automaton::core::network;  // NOLINT

std::shared_ptr<connection> connection_c;

class test_server_handler: public server::server_handler {
 public:
  test_server_handler() {}
  std::string handle(std::string s) {
    return s + "response";
  }
};

class client_handler:public connection::connection_handler {
 public:
  void on_message_received(connection_id c, std::shared_ptr<char> buffer, uint32_t bytes_read, uint32_t mid) {
    std::string message = std::string(buffer.get(), bytes_read);
    LOG(INFO) << "Message \"" << message << "\" received from server";
    EXPECT_EQ(message, "requestresponse");
    // connection_c->async_read(buffer, 256, 0, 0);
  }
  void on_message_sent(connection_id c, uint32_t mid, const automaton::core::common::status& s) {
    if (s.code != automaton::core::common::status::OK) {
      LOG(INFO) << "Message was NOT sent to server";
    } else {
      LOG(INFO) << "Message was successfully sent to server";
    }
  }
  void on_connected(connection_id c) {
    LOG(INFO) << "Connected with server";
  }
  void on_disconnected(connection_id c) {
    LOG(INFO) << "Disconnected with server";
  }
  void on_connection_error(connection_id c, const automaton::core::common::status& s) {
    if (s.code == automaton::core::common::status::OK) {
      return;
    }
    LOG(ERROR) << s;
  }
};

std::shared_ptr<char> buffer_c = std::shared_ptr<char>(new char[256], std::default_delete<char[]>());

std::shared_ptr<client_handler> handler_c;

void client() {
  connection_c = connection::create("tcp", 1, SERVER_ADDRESS, std::move(handler_c));
  if (connection_c->init()) {
    LOG(DEBUG) << "Connection init was successful!";
    connection_c -> connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_read(buffer_c, 256, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    connection_c -> async_send("request", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    connection_c -> disconnect();
  } else {
    LOG(ERROR) << "Connection init failed!";
  }
}

TEST(rpc_echo, basic_test) {
  automaton::core::network::tcp_init();
  std::shared_ptr<server::server_handler> handler(new test_server_handler());
  automaton::core::network::server rpc(PORT, handler);
  rpc.run();
  client();
  rpc.stop();
  automaton::core::network::tcp_release();
}
