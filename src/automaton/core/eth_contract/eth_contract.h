#ifndef AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_
#define AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace eth_contract {


// Helper encode/decode functions

std::string hash(const std::string& data);

std::string dec_to_32hex(uint32_t n);

class eth_contract: public automaton::core::network::connection::connection_handler,
    public std::enable_shared_from_this<eth_contract> {
 public:
  static std::unordered_map<std::string, std::shared_ptr<eth_contract> > contracts;

  static void register_contract(const std::string& server, const std::string& address, std::vector<std::string> signs);
  static std::shared_ptr<eth_contract> get_contract(const std::string&);

  ~eth_contract();

  void call(const std::string& address, const std::string& f,
      const std::string& params,
      std::function<void(const automaton::core::common::status& s, const std::string&)>);

 private:
  uint32_t call_id;
  std::string server;
  std::string address;  // ETH address of the contract
  std::unordered_map<std::string, std::string> signatures;  // function signatures and
  std::unordered_map<uint32_t,
      std::function<void(const automaton::core::common::status&, const std::string&)> > callbacks;

  std::shared_ptr<automaton::core::network::connection> conn;
  // std::string header;
  // std::string body;
  // std::string message;

  uint32_t buffer_size;
  std::shared_ptr<char> buffer;

  eth_contract(const std::string& server, const std::string& address, std::vector<std::string> signatures);

  // void read_header();
  // void read_body();
  // void handle_message(const std::string&);

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
