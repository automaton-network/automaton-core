#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>  // NOLINT
#include <json.hpp>

#include "automaton/core/network/connection.h"

static const uint32_t ERROR_BUF_SIZE = 1024;
static const uint32_t MSG_BUF_SIZE = 1024;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

/* TODO(kari):
  * Create transaction in call() and fill the values.
  * Transaction default field values (gas_price, chain_id) and values taken automatically from the blockchain (nonce,
gas_limit).
*/

// Helper encode/decode functions

std::string hash(const std::string& data);

std::string dec_to_32hex(uint32_t n);

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
    @param[in] abi_json contract abi as json string
  */

  static void register_contract(const std::string& server, const std::string& address,
      const std::string& abi_json);

  static std::shared_ptr<eth_contract> get_contract(const std::string&);

  eth_contract(const std::string& server, const std::string& address, const std::string& abi_json);

  ~eth_contract();

  /**
    @param[in] f function name/alias as given in register_contract
    @param[in] params concatenated function parameters where every parameter is padded to 32
    @returns status code
  */
  common::status call(const std::string& f, const std::string& params);

 private:
  uint32_t call_id;
  std::string server;
  std::string address;  // ETH address of the contract
  nlohmann::json abi;
  std::unordered_map<std::string, std::pair<std::string, bool> > signatures;  // function signatures
  struct curl_slist *list = NULL;

  CURL *curl;
  CURLcode res;
  std::string message;

  char curl_err_buf[ERROR_BUF_SIZE];

  static std::unordered_map<std::string, std::pair<std::string, bool> > parse_abi(nlohmann::json json_abi);

  static size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s);

  common::status handle_message();
};

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_
