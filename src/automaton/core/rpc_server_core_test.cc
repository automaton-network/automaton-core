#include <memory>
#include <string>

#include "automaton/core/network/tcp_implementation.h"

static const char* SERVER_ADDRESS = "127.0.0.1:55666";
using namespace automaton::core::network;  //  NOLINT

std::shared_ptr<connection> connection_c;

class client_handler:public connection::connection_handler {
 public:
  void on_message_received(connection_id c, char* buffer, uint32_t bytes_read, uint32_t mid) {
    std::string message = std::string(buffer, bytes_read);
    std::cout << "Message \n\"" << message << "\"\nreceived from server" << std::endl;
    connection_c->async_read(buffer, 256, 0, 0);
  }
  void on_message_sent(connection_id c, uint32_t mid, const automaton::core::common::status& s) {
    if (s.code != automaton::core::common::status::OK) {
      std::cout << "Command was NOT sent to server" << std::endl;
    } else {
      std::cout << "Command was successfully sent to server" << std::endl;
    }
  }
  void on_connected(connection_id c) {
    std::cout << "Connected with server" << std::endl;
  }
  void on_disconnected(connection_id c) {
    std::cout << "Disconnected with server" << std::endl;
  }
  void on_connection_error(connection_id c, const automaton::core::common::status& s) {
    if (s.code == automaton::core::common::status::OK) {
      return;
    }
    std::cout << s << std::endl;
  }
};

char* buffer_c = new char[256];

client_handler handler_c;

void client() {
  connection_c = connection::create("tcp", 1, SERVER_ADDRESS, &handler_c);
  if (connection_c->init()) {
    std::cout << "Connection init was successful!" << std::endl;
    connection_c -> connect();
    connection_c -> async_read(buffer_c, 256, 0, 0);
    std::string input;
    while (std::getline(std::cin, input)) {
      std::stringstream ss;
      ss << "GET /hello.htm HTTP/1.1\r\n";
      ss << "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n";
      ss << "Host: localhost\r\n";
      ss << "Accept-Language: en-us\r\n";
      ss << "Accept-Encoding: gzip, deflate\r\n";
      ss << "Content-Length: " << input.size() << "\r\n";
      ss << "Connection: Keep-Alive\r\n";
      ss << "\r\n" << input;

      connection_c -> async_send(ss.str(), 0);
    }
    connection_c -> disconnect();
  } else {
    std::cout << "Connection init failed!" << std::endl;
  }
}

int main(int argc, char* argv[]) {
  automaton::core::network::tcp_init();
  client();
  automaton::core::network::tcp_release();
  delete [] buffer_c;
  return 0;
}
