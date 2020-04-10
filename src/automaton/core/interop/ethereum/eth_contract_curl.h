#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>  // NOLINT
#include <json.hpp>

#include "automaton/core/interop/ethereum/eth_transaction.h"
#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

/* TODO(kari): Extract transaction result from transaction receipt. Right now if you call a function that is a transaction and returns value, the only way to get the returned value is to check the transaction receipt
*/

// Helper encode/decode functions

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
    @param[in] fname function name/alias as given in register_contract
    @param[in] params
      case 1: sending raw transaction -> params is signed transaction
      case 2: params is list in json format where 'bytes' and 'address' are given in hex, integers in decimal,
      'string' is string, 'boolean' is "true" or "false" (without quotes)
      ! 'fixed' numbers are not yet supported !
    @param[in] nonce in hex (without 0x prefix)
    @param[in] gas_price in hex (without 0x prefix)
    @param[in] gas_limit in hex (without 0x prefix)
    @param[in] value in hex (without 0x prefix)
    @param[in] private_key in hex (without 0x prefix)
    @returns status code where msg contains the function result -
      transaction receipt if the function is a transaction, decoded function result in json format, otherwise
  */
  common::status call(const std::string& fname, const std::string& params,
      const std::string& private_key = "", const std::string& value = "",
      const std::string& gas_price = "", const std::string& gas_limit = "");

  void set_gas_price(const std::string& new_gas_price_hex);
  void set_gas_limit(const std::string& new_gas_limit_hex);

 private:
  uint32_t call_ids;
  std::mutex call_ids_mutex;

  std::string gas_limit = "5B8D80";  // 6M
  std::string gas_price = "1388";  // 5k

  std::string server;
  std::string address;  // ETH address of the contract
  nlohmann::json abi;
  std::unordered_map<std::string, std::pair<std::string, bool> > signatures;  // function signatures
  std::unordered_map<std::string, std::string> function_inputs;
  std::unordered_map<std::string, std::string> function_outputs;

  void parse_abi(nlohmann::json json_abi);
  common::status decode_function_result(const std::string& fname, const std::string& result);
  // common::status decode_transaction_receipt(const std::string& receipt);  // Extract function result from logs
  uint32_t get_next_call_id();
};

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_CONTRACT_CURL_H_
