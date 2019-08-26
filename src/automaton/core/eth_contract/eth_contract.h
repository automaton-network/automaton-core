#ifndef AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_
#define AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace eth_contract {

/*
  TODO(kari):
    * operator[] overload
    * is http response always chunked?
    * get rid of recursion in read_header() and read_body()
    * close connection and clear buffers if error occurs and send error messages to all callbacks or resend requests
    * handle all possible solidity/json responses
*/

// Helper encode/decode functions

std::string hash(const std::string& data);

std::string dec_to_32hex(uint32_t n);

uint32_t hex_to_dec(const std::string& hex);

/**
  Class storing Ethereum contract address and function signatures.
  It is used to send eth_call requests to the Ethereum network and pass the result to a given callback function.
  This class is NOT thread safe. If functions are called simultaneously, it won't work correctly.
*/

class eth_contract: public automaton::core::network::connection::connection_handler,
    public std::enable_shared_from_this<eth_contract> {
 public:
  static std::unordered_map<std::string, std::shared_ptr<eth_contract> > contracts;

  static void register_contract(const std::string& server, const std::string& address,
      std::unordered_map<std::string, std::pair<std::string, bool> > signs);
  static std::shared_ptr<eth_contract> get_contract(const std::string&);

  ~eth_contract();

  void call(const std::string& address, const std::string& f, const std::string& params,
      std::function<void(const automaton::core::common::status& s, const std::string&)>);

 private:
  uint32_t call_id;
  std::string server;
  std::string address;  // ETH address of the contract
  std::unordered_map<std::string, std::pair<std::string, bool> > signatures;  // function signatures
  std::unordered_map<uint32_t,
      std::function<void(const automaton::core::common::status&, const std::string&)> > callbacks;

  std::shared_ptr<automaton::core::network::connection> conn;
  std::string header;
  std::string body;
  std::string message;

  uint32_t buffer_size;
  std::shared_ptr<char> buffer;

  eth_contract(const std::string& server, const std::string& address,
      std::unordered_map<std::string, std::pair<std::string, bool> > signatures);

  void handle_message(const automaton::core::common::status& s);

  void read_header();

  void read_body();

  // Inherited from connection_handler

  void on_message_received(automaton::core::network::connection_id c,
      std::shared_ptr<char> buffer, uint32_t bytes_read, uint32_t mid);

  void on_message_sent(automaton::core::network::connection_id c, uint32_t mid,
      const automaton::core::common::status& s);

  void on_connected(automaton::core::network::connection_id c);

  void on_disconnected(automaton::core::network::connection_id c);

  void on_connection_error(automaton::core::network::connection_id c,
      const automaton::core::common::status& s);
};

}  // namespace eth_contract
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_
