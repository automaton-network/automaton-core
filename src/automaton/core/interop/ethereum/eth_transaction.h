#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_TRANSACTION_H_

#include <string>

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

/**
 Returns Keccak_256 hash of data as 32-byte string.
*/
std::string hash(const std::string& data);

/**
 Returns RLP encoding of s in hex.
 @param[in] s data to be encode, MUST be in hex WITHOUT '0x' prefix.
 @param[in] is_list shows if s represents one element or the payload of a list.
*/
std::string rlp_encode(std::string s, bool is_list);

/**
 Signs a message using secp256k1 also checks if a public key can be created from the given private key. If the check
 fails, empty string will be returned. Returns concatenated r, s and v values of the signature.
 @param[in] priv_key byte array repesenting the private key
 @param[in] message_hash 32-byte string.
*/
std::string check_and_sign(const unsigned char* priv_key, const unsigned char* message_hash);

/**
 Returns recovered Ethereum address from a message and a signature.
*/
std::string recover_address(const unsigned char* rsv, const unsigned char* message_hash);

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
