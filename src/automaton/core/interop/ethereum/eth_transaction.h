#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_

#include <string>

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

std::string hash(const std::string& data);

std::string rlp_encode(std::string s, bool is_list);

std::string check_and_sign(const unsigned char* priv_key, const unsigned char* message);

std::string recover_address(const unsigned char* rsv, const unsigned char* message_hash);

/**
  Ethereum transaction. Transaction fields should be given as hex strings WITHOUT '0x' prefix.
*/
class eth_transaction {
 public:
  std::string nonce;
  std::string gasPrice;
  std::string gasLimit;
  std::string to;
  std::string value;
  std::string data;
  std::string chainId;

  std::string sign_tx(const std::string& private_key_hex);

 private:
  std::string to_rlp_encoded_tx();
};


}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_
