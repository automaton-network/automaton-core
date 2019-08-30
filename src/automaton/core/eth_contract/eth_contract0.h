#ifndef AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_
#define AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>  // NOLINT

#include "automaton/core/network/connection.h"

static const uint32_t ERROR_BUF_SIZE = 1024;
static const uint32_t MSG_BUF_SIZE = 1024;

namespace automaton {
namespace core {
namespace eth_contract {

// Helper encode/decode functions

std::string hash(const std::string& data);

std::string dec_to_32hex(uint32_t n);

uint32_t hex_to_dec(const std::string& hex);

/**
  Class storing Ethereum contract address and function signatures.
  It is used to send eth_call requests to the Ethereum network and pass the result to a given callback function.
  This class is NOT thread safe. If functions are called simultaneously, it won't work correctly.
*/

class eth_contract: public std::enable_shared_from_this<eth_contract> {
 public:
  static std::unordered_map<std::string, std::shared_ptr<eth_contract> > contracts;

  /**
    @param[in] server ip address or url
    @param[in] address contract address on ethereum main or testnet
    @param[in] signs map of function name/alias -> {function signature, true/false is transaction}
        e.g. {"getSlotsNumber", {"getSlotsNumber()", false}}
  */
  static void register_contract(const std::string& server, const std::string& address,
      std::unordered_map<std::string, std::pair<std::string, bool> > signs);
  static std::shared_ptr<eth_contract> get_contract(const std::string&);

  ~eth_contract();

  /**
    @param[in] address public key from which transaction is sent; not needed if called funcion is not transaction
    @param[in] f function name/alias as given in register_contract
    @param[in] params concatenated function parameters where every parameter is padded to 32
    @param[in] callback function to be called when result from the transaction/eth function call is received or error
        happen.
  */
  void call(const std::string& address, const std::string& f, const std::string& params,
      std::function<void(const automaton::core::common::status& s, const std::string& result)> callback);

 private:
  uint32_t call_id;
  std::string server;
  std::string address;  // ETH address of the contract
  std::unordered_map<std::string, std::pair<std::string, bool> > signatures;  // function signatures
  std::unordered_map<uint32_t,
      std::function<void(const automaton::core::common::status&, const std::string&)> > callbacks;

  CURL *curl;
  CURLcode res;
  std::string message;

  char curl_err_buf[ERROR_BUF_SIZE];

  static size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s);

  eth_contract(const std::string& server, const std::string& address,
      std::unordered_map<std::string, std::pair<std::string, bool> > signatures);

  void handle_message(const automaton::core::common::status& s);

  std::string create_raw_transaction();
};

}  // namespace eth_contract
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_ETH_CONTRACT_ETH_CONTRACT_H_
