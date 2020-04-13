#include "automaton/core/interop/ethereum/eth_transaction.h"

#include <iostream>  // for debugging purposes / to be removed

#include "automaton/core/interop/ethereum/eth_helper_functions.h"
#include "automaton/core/io/io.h"

using automaton::core::io::bin2hex;
using automaton::core::io::dec2hex;
using automaton::core::io::hex2bin;
using automaton::core::io::hex2dec;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

std::string eth_transaction::sign_tx(const std::string& private_key_hex) {
  std::string rlp_tx = to_rlp_encoded_tx();
  std::string extended_sign_tx = rlp_encode(rlp_tx + rlp_encode(chain_id, false) + "8080", true);
  std::string byte_array = hex2bin(extended_sign_tx);
  // std::cout << "BEFORE HASH tx hex: " << extended_sign_tx << std::endl;
  std::string hashed_tx = hash(byte_array);
  // std::cout << "HASH tx hex: " << bin2hex(hashed_tx) << std::endl;
  std::string pr_key = hex2bin(private_key_hex);
  std::string rsv = secp256k1_sign_and_verify(reinterpret_cast<const unsigned char*>(pr_key.c_str()),
      reinterpret_cast<const unsigned char*>(hashed_tx.c_str()));
  int32_t v = rsv[64];
  v -= 27;
  // std::cout << "ADDRESS: " << bin2hex(recover_address(reinterpret_cast<const unsigned char*>(rsv.c_str()),
      // reinterpret_cast<const unsigned char*>(hashed_tx.c_str()))) << std::endl;
  std::stringstream tx;
  uint32_t newv = hex2dec(chain_id) * 2 + ((v % 2) ? 36 : 35);
  tx << rlp_tx << rlp_encode(dec2hex(newv), false) << "a0" << bin2hex(rsv.substr(0, 32)) <<
      "a0" << bin2hex(rsv.substr(32, 32));
  // std::cout << "\nSIGNED TX:\n" << tx.str() << std::endl;
  return rlp_encode(tx.str(), true);
}

std::string eth_transaction::to_rlp_encoded_tx() {
  std::stringstream tx;
  tx << rlp_encode(nonce, false) <<
      rlp_encode(gas_price, false) <<
      rlp_encode(gas_limit, false) <<
      rlp_encode(to, false) <<
      rlp_encode(value, false) <<
      rlp_encode(data, false);
  return tx.str();
}

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton
