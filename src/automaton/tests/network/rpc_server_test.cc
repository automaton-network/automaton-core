#include "automaton/core/network/rpc_server.h"
#include "automaton/core/network/tcp_implementation.h"
#include <memory>
#include <string>
//#include <cstdint>
#include "gtest/gtest.h"

static const uint32_t PORT = 33445;
static const char* SERVER_ADDRESS = "127.0.0.1:33445";
using namespace automaton::core::network;

std::shared_ptr<connection> connection_c;

std::string server_handler(std::string test_str){
  return test_str + "response";
}

class client_handler:public connection::connection_handler {
 public:
  void on_message_received(connection_id c, char* buffer, uint32_t bytes_read, uint32_t mid) {
    std::string message = std::string(buffer, bytes_read);
    LOG(INFO) << "Message \"" << message << "\" received from server";
    connection_c->async_read(buffer, 256, 0, 0);
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

char* buffer_c = new char[256];

client_handler handler_c;

void client() {
  connection_c = connection::create("tcp", 1, SERVER_ADDRESS, &handler_c);
  if (connection_c->init()) {
    LOG(DEBUG) << "Connection init was successful!";
    connection_c -> connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_read(buffer_c, 256, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("firstrequest", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("secondrequest", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    connection_c -> disconnect();
  } else {
    LOG(ERROR) << "Connection init failed!";
  }
}

TEST(rpc_echo, basic_test) {
  automaton::core::network::tcp_init();
  automaton::core::network::server rpc(PORT, &server_handler);
  rpc.run();
  client();
  rpc.stop();
  automaton::core::network::tcp_release();
  delete [] buffer_c;
}
