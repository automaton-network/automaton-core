#include "automaton/core/network/http_server.h"
#include <memory>
#include <string>
#include "automaton/core/network/tcp_implementation.h"

static const uint32_t PORT = 33445;
static const char* SERVER_ADDRESS = "127.0.0.1:33445";
using namespace automaton::core::network;  // NOLINT

std::shared_ptr<connection> connection_c;

class test_server_handler: public http_server::server_handler {
 public:
  test_server_handler() {}
  std::string handle(std::string request, http_server::status_code* s) {
    *s = http_server::status_code::OK;
    return request + "response";
  }
};

class client_handler:public connection::connection_handler {
 public:
  void on_message_received(connection_id c, char* buffer, uint32_t bytes_read, uint32_t mid) {
    std::string message = std::string(buffer, bytes_read);
    LOG(INFO) << "Message \"" << message << "\" received from server";
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

char* buffer_c = new char[256];

client_handler handler_c;

void client() {
  connection_c = connection::create("tcp", 1, SERVER_ADDRESS, &handler_c);
  if (connection_c->init()) {
    LOG(DEBUG) << "Connection init was successful!";
    connection_c -> connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_read(buffer_c, 256, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::stringstream ss;
    ss << "GET /hello.htm HTTP/1.1\r\n";
    ss << "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n";
    ss << "Host: localhost\r\n";
    ss << "Accept-Language: en-us\r\n";
    ss << "Accept-Encoding: gzip, deflate\r\n";
    ss << "Content-Length: 7\r\n";
    ss << "Connection: Keep-Alive\r\n";
    ss << "\r\n" << "request";

    connection_c -> async_send(ss.str(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    connection_c -> disconnect();
  } else {
    LOG(ERROR) << "Connection init failed!";
  }
}

int main() {
  automaton::core::network::tcp_init();
  std::shared_ptr<http_server::server_handler> handler(new test_server_handler());
  automaton::core::network::http_server rpc(PORT, handler);
  rpc.run();
  client();
  rpc.stop();
  automaton::core::network::tcp_release();
  delete [] buffer_c;
  return 0;
}
