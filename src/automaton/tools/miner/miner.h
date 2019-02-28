#ifndef AUTOMATON_TOOLS_MINER_MINER_H_
#define AUTOMATON_TOOLS_MINER_MINER_H_

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "secp256k1/include/secp256k1_recovery.h"
#include "secp256k1/include/secp256k1.h"
#include "secp256k1/src/hash_impl.h"
#include "secp256k1/src/hash.h"

// will need tests, so we need to make it library,
// but we also need it to be executable printing the information
// in the correct format

namespace automaton {
namespace tools {
namespace miner {

std::string str_tohex(std::string s) {
  std::stringstream ss;
  for (int i = 0; i < s.size(); i++) {
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (static_cast<int>(s[i]) & 0xff);
  }
  return ss.str();
}

bool mine_key(unsigned char* mask, unsigned char* difficulty) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  std::string pub_key_after_mask(32, '0');
  unsigned char priv_key[32];
  bool found = false;
  unsigned char * msg_hash = new unsigned char[32];
  memset(msg_hash, 0, 32);

  // unsigned char mask[32] = { 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
  //                            0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  //                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  //                            0xFF, 0xFF };
  // unsigned char difficulty[32] = { 0x80, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
  //                                  0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
  //                                  0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
  //                                  0x00, 0x00 };

  std::random_device engine;


  while (!found) {
    for (int i = 0; i < 32; i++) {
      priv_key[i] = engine();
    }
    secp256k1_ec_pubkey_create(context, pubkey, priv_key);

    unsigned char pub_key_serialized[65];
    size_t outLen = 65;
    secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);

    std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);
    std::string pub_key_x(reinterpret_cast<char*>(pub_key_serialized+1), 32);
    std::string pub_key_y(reinterpret_cast<char*>(pub_key_serialized+33), 32);

    for (int i = 0; i < pub_key_x.size(); i++) {
      pub_key_after_mask[i] = pub_key_x[i] ^ mask[i];
    }
    std::cout << "pub_key: " << str_tohex(pub_key_uncompressed) << std::endl;
    std::cout << "pub_key_x: " << str_tohex(pub_key_x) << std::endl;
    std::cout << "after_msk: " << str_tohex(pub_key_after_mask) << std::endl;
    std::cout << "pub_key_y: " << str_tohex(pub_key_y) << std::endl;


    if (memcmp(difficulty, pub_key_after_mask.data(), 32) < 0) {
      std::cout << std::endl << "Valid key found!!!" << std::endl << std::endl;
      break;
    }
  }
  return 0;
}


}  // namespace miner
}  // namespace tools
}  // namespace automaton

#endif  //  AUTOMATON_TOOLS_MINER_MINER_H_
