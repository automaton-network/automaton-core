#include <assert.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "secp256k1/include/secp256k1_recovery.h"
#include "secp256k1/include/secp256k1.h"
#include "secp256k1/src/hash_impl.h"
#include "secp256k1/src/hash.h"
#include "automaton/tools/miner/miner.h"
#include "automaton/core/io/io.h"


using automaton::tools::miner::mine_key;
using automaton::tools::miner::sign;
using automaton::core::io::hex2bin;
using automaton::core::io::bin2hex;

int main(int argc, char* argv[]) {
  unsigned char mask[32] = {0};
  unsigned char difficulty[32] = {0};
  unsigned char priv_key[32] = {0};
  unsigned char address[32] = {0};
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  std::string mask_input, difficulty_input, address_input;
  // Input the mask, difficulty and hash of the benefactors address
  std::cout << "Input should be up to 64 HEX values. When it's less than that, the value of the "
        "mask and difficulty will be set as prefix, address as sufix"
            << std::endl;
  std::cout << "mask: ";
  std::cin >> mask_input;
  std::cout << "difficulty prefix: ";
  std::cin >> difficulty_input;
  std::cout << "address: ";
  std::cin >> address_input;

  std::string mask_bin = hex2bin(mask_input);
  std::string difficulty_bin = hex2bin(difficulty_input);
  std::string address_bin = hex2bin(address_input);

  for (int i = 0; i < mask_bin.size(); i++) {
    mask[i] = mask_bin[i];
  }
  for (int i = 0; i < difficulty_bin.size(); i++) {
    difficulty[i] = difficulty_bin[i];
  }
  for (int i = 0; i < address_bin.size(); i++) {
    int curr = 32 - address_bin.size() + i;
    address[curr] = address_bin[i];
  }

  while (1) {
    bool mined = mine_key(mask, difficulty, priv_key);
    if (mined) {
      std::stringstream ss;
      bin2hex(std::string(reinterpret_cast<char*>(priv_key)));
      std::string rsv = sign(priv_key, address);
      ss << "rsv: " << bin2hex(rsv) << std::endl;

      secp256k1_ec_pubkey_create(context, pubkey, priv_key);
      unsigned char pub_key_serialized[65];
      size_t outLen = 65;
      secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);
      std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);
      std::string pub_key_x(reinterpret_cast<char*>(pub_key_serialized+1), 32);
      std::string pub_key_y(reinterpret_cast<char*>(pub_key_serialized+33), 32);
      std::cout << "koh.claimSlot('0x" << bin2hex(pub_key_x) << "', '0x" << bin2hex(pub_key_y) << "', '0x"
                << bin2hex(std::string(reinterpret_cast<char*>(address+12), 20)) << "', '0x"
                << bin2hex(rsv.substr(0, 32)) << "', '0x" << bin2hex(rsv.substr(32, 32)) << "', '0x"
                << bin2hex(rsv.substr(64, 1)) << "')"<< std::endl;
    }
  }


  return 0;
}
