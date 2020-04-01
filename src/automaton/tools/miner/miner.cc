#include "automaton/tools/miner/miner.h"

#include <assert.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "automaton/core/io/io.h"

using automaton::core::io::bin2hex;

// will need tests, so we need to make it library,
// but we also need it to be executable printing the information
// in the correct format
// TODO(NOW): Create the sign function, Create cc_binary using the create_key and sign function printing the result.
//            Add tests for sign and binary, upload.
namespace automaton {
namespace tools {
namespace miner {

std::string str_tohex(std::string s) {
  std::stringstream ss;
  for (size_t i = 0; i < s.size(); i++) {
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (static_cast<int>(s[i]) & 0xff);
  }
  return ss.str();
}

unsigned int mine_key(unsigned char* mask, unsigned char* difficulty, unsigned char* priv_key, int max_attempts) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  std::string pub_key_after_mask(32, '0');
  std::random_device engine;
  bool found = false;

  unsigned int keys_generated = 0;

  while (!found && (--max_attempts > 0)) {
    for (int i = 0; i < 32; i++) {
      priv_key[i] = static_cast<uint8_t>(engine());
    }
    if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
      LOG(WARNING) << "Invalid priv_key " << bin2hex(std::string(reinterpret_cast<char*>(priv_key), 32));
      continue;
    }
    keys_generated++;

    unsigned char pub_key_serialized[65];
    size_t outLen = 65;
    secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);

    std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);

    for (int i = 0; i < 32; i++) {
        pub_key_after_mask[i] = pub_key_uncompressed[i+1] ^ mask[i];
    }

    if (memcmp(difficulty, pub_key_after_mask.data(), 32) < 0) {
      found = true;
      break;
    }
  }
  delete pubkey;
  secp256k1_context_destroy(context);
  return keys_generated;
}

std::string sign(const unsigned char* priv_key, const unsigned char* msg_hash) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature signature;
  char signatureArr[65];
  int v = -1;
  secp256k1_ecdsa_sign_recoverable(context, &signature, (unsigned char*)msg_hash, (unsigned char*)priv_key, NULL, NULL);
  secp256k1_ecdsa_recoverable_signature_serialize_compact(context, (unsigned char*)signatureArr, &v, &signature);
  secp256k1_context_destroy(context);
  signatureArr[64] = static_cast<uint8_t>(v) + 27;
  return std::string(reinterpret_cast<char*>(signatureArr), 65);
}

std::string gen_pub_key(const unsigned char* priv_key) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
    LOG(WARNING) << "Invalid priv_key " << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32));
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }

  unsigned char pub_key_serialized[65];
  size_t outLen = 65;
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);
  delete pubkey;
  secp256k1_context_destroy(context);
  return std::string(reinterpret_cast<char*>(&pub_key_serialized[1]), outLen);
}

}  // namespace miner
}  // namespace tools
}  // namespace automaton
