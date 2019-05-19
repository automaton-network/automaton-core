#include <string>
#include <thread>

#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/io/io.h"

using automaton::core::network::acceptor;
using automaton::core::network::connection;
using automaton::core::network::connection_id;
using automaton::core::network::acceptor_id;
using automaton::core::common::status;

const char* address_a = "127.0.0.1:12333";
const char* address_b = "127.0.0.1:12366";

std::map<uint32_t, std::shared_ptr<automaton::core::network::connection> > connections;

int counter = 100;
static connection_id cids = 0;
std::mutex ids_mutex;

bool running = true;

uint32_t get_new_id() {
  std::lock_guard<std::mutex> lock(ids_mutex);
  return ++cids;
}

std::shared_ptr<char> bufferC = std::shared_ptr<char>(new char[256], std::default_delete<char[]>());

class handler: public connection::connection_handler {
 public:
  void on_message_received(connection_id c, std::shared_ptr<char> buffer, uint32_t bytes_read, uint32_t mid) {
    std::string message = std::string(buffer.get(), bytes_read);
    LOG(INFO) << "Message \"" << message << "\" received from " << c;
    if (message.compare("Thank you!")) {
      connections[c]->async_send("Thank you!", counter++);
    }
    connections[c]->async_read(buffer, 256, 0);
  }
  void on_message_sent(connection_id c, uint32_t mid, const status& s) {
    if (s.code != status::OK) {
      LOG(INFO) << "Message with id " << std::to_string(mid) << " was NOT sent to " << c << "\nERROR: " << s;
    } else {
      LOG(INFO) << "Message with id " << std::to_string(mid) << " was successfully sent to " << c;
    }
  }
  void on_connected(connection_id c) {
    LOG(INFO) << "Connected with: " << c;
  }
  void on_disconnected(connection_id c) {
    LOG(INFO) << "Disconnected with: " << c;
  }
  void on_connection_error(connection_id c, const status& s) {
    if (s.code == status::OK) {
      return;
    }
    LOG(ERROR) << s << " (connection " << c << ")";
  }
};

class lis_handler: public acceptor::acceptor_handler {
 public:
  // TODO(kari): Add constructor that accepts needed options
  // (vector connections, max ...)
  bool on_requested(acceptor_id a, const std::string& address, uint32_t* pid) {
  //  EXPECT_EQ(address, address_a);
    *pid = get_new_id();
    LOG(INFO) << "Connection request from: " << address << ". Accepting...";
    return true;
  }
  void on_connected(acceptor_id a, std::shared_ptr<connection> c, const std::string& address) {
    LOG(INFO) << "Accepted connection from: " << address;
    connections[c->get_id()] = c;
    std::shared_ptr<char> buffer = std::shared_ptr<char>(new char[256], std::default_delete<char[]>());
    c->async_read(buffer, 256, 0);
  }
  void on_acceptor_error(acceptor_id a, const status& s) {
    LOG(ERROR) << "Acceptor ERROR " << s;
  }
};

std::shared_ptr<handler> handlerC = std::make_shared<handler>();
std::shared_ptr<handler> handlerA = std::make_shared<handler>();

void func() {
  std::shared_ptr<connection> connection_c = connection::create("tcp", get_new_id(), address_a, std::move(handlerC));
  if (connection_c->init()) {
    connections[connection_c->get_id()] = connection_c;
    LOG(DBUG) << "Connection init was successful!";
    connection_c -> connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_read(bufferC, 256, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("C0", 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("C1", 4);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    connection_c -> async_send("C2", 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    connection_c -> disconnect();
    connection_c -> connect();
    connection_c -> async_read(bufferC, 256, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("C3", 6);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    connection_c -> async_send("C4", 7);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    connection_c -> async_send("C5", 8);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  } else {
    LOG(ERROR) << "Connection init failed!";
  }
}

int main() {
  automaton::core::network::tcp_init();
  std::shared_ptr<lis_handler> l_handler = std::make_shared<lis_handler>();
  std::shared_ptr<acceptor> acceptorB =
      acceptor::create("tcp", 1, address_a, std::move(l_handler), std::move(handlerA));
  if (acceptorB->init()) {
    LOG(DBUG) << "Acceptor init was successful!";
    acceptorB->start_accepting();
    LOG(DBUG) << acceptorB.use_count() << " -> " << acceptorB->get_state();
  } else {
    LOG(ERROR) << "Acceptor init failed!";
  }
  std::thread t(func);

  std::this_thread::sleep_for(std::chrono::milliseconds(2500));
  t.join();
  automaton::core::network::tcp_release();
  connections.clear();
  return 0;
}
