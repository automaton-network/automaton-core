#include "automaton/core/interop/ethereum/eth_transaction.h"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <iostream>  // for debugging purposes / to be removed

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/io/io.h"
#include "automaton/tools/miner/miner.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::interop::ethereum::hash;
using automaton::core::io::bin2hex;
using automaton::core::io::dec2hex;
using automaton::core::io::hex2bin;
using automaton::core::io::hex2dec;
using automaton::tools::miner::sign;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

std::string hash(const std::string& data) {
  Keccak_256_cryptopp hasher;
  uint8_t digest[32];
  hasher.calculate_digest(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  return std::string(reinterpret_cast<char*>(digest), 32);
}

std::string rlp_encode(std::string s, bool is_list) {
  if (s.size() % 2) {
    s = "0" + s;
  }
  uint32_t length = static_cast<uint32_t>(s.size()) / 2;
  if (length == 0) {
    return "80";
  }
  if (length == 1 && hex2dec(s.substr(0, 2)) < 128) {
    return s;
  }
  std::stringstream ss;
  if (length < 56) {
    ss << dec2hex((is_list ? 192 : 128) + length) << s;
  } else {
    std::string length_in_hex = dec2hex(length);
    uint32_t first_byte = (is_list ? 247 : 183) + (static_cast<uint32_t>(length_in_hex.size()) / 2);
    // todo: check if this byte is valid
    ss << dec2hex(first_byte) << length_in_hex << s;
  }
  return ss.str();
}

std::string check_and_sign(const unsigned char* priv_key, const unsigned char* message_hash) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  std::string rsv = sign(priv_key, message_hash);

  if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
    LOG(WARNING) << "Invalid private key!!!" << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32));
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }
  delete pubkey;
  secp256k1_context_destroy(context);
  return rsv;
}

std::string recover_address(const unsigned char* rsv, const unsigned char* message_hash) {
  int32_t v = rsv[64];
  v -= 27;
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature signature;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(context, &signature, (unsigned char*)rsv, v)) {
    LOG(WARNING) << "Cannot parse signature!";
    secp256k1_context_destroy(context);
    return "";
  }
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  if (!secp256k1_ecdsa_recover(context, pubkey, &signature, (unsigned char*) message_hash)) {
    LOG(WARNING) << "Cannot recover signature!";
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }

  size_t out_len = 65;
  unsigned char pub_key_serialized[65];
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &out_len, pubkey, SECP256K1_EC_UNCOMPRESSED);
  std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), out_len);
  delete pubkey;
  secp256k1_context_destroy(context);
  return hash(pub_key_uncompressed.substr(1)).substr(12);
}

std::string eth_transaction::sign_tx(const std::string& private_key_hex) {
  std::string rlp_tx = to_rlp_encoded_tx();
  std::string extended_sign_tx = rlp_encode(rlp_tx + rlp_encode(chain_id, false) + "8080", true);
  std::string byte_array = hex2bin(extended_sign_tx);
  // std::cout << "BEFORE HASH tx hex: " << extended_sign_tx << std::endl;
  std::string hashed_tx = hash(byte_array);
  // std::cout << "HASH tx hex: " << bin2hex(hashed_tx) << std::endl;
  std::string pr_key = hex2bin(private_key_hex);
  std::string rsv = check_and_sign(reinterpret_cast<const unsigned char*>(pr_key.c_str()),
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
