#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_

#include <string>

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

/**
  Ethereum transaction. Transaction fields should be given as hex strings WITHOUT '0x' prefix.
*/
class eth_transaction {
 public:
  std::string nonce;
  std::string gas_price;
  std::string gas_limit;
  std::string to;
  std::string value;
  std::string data;
  std::string chain_id;

  /**
   Constructs and signs the transaction with the given in hex private_key then adds the signature and encode it in RLP.
   The result is the whole raw transaction in hex ready to be uploaded.
  */
  std::string sign_tx(const std::string& private_key_hex);

 private:
  std::string to_rlp_encoded_tx();
};


}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton


#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_
