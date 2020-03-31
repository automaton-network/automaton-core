#include <assert.h>
#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "automaton/tools/miner/miner.h"
#include "automaton/core/io/io.h"

using automaton::tools::miner::mine_key;
using automaton::tools::miner::sign;
using automaton::core::io::hex2bin;
using automaton::core::io::bin2hex;

void check_and_sign(const unsigned char* priv_key, const unsigned char* address) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  std::string rsv = sign(priv_key, address);

  if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
    LOG(WARNING) << "Invalid priv_key " << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32));
    return;
  }

  unsigned char pub_key_serialized[65];
  size_t outLen = 65;
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);
  std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);
  std::string pub_key_x(reinterpret_cast<char*>(pub_key_serialized+1), 32);
  std::string pub_key_y(reinterpret_cast<char*>(pub_key_serialized+33), 32);
  // std::cout << "PrvKey: " << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32)) << std::endl;
  std::cout << "koh.claimSlot('0x" << bin2hex(pub_key_x) << "', '0x" << bin2hex(pub_key_y)
            // << "', '0x" << bin2hex(std::string(reinterpret_cast<const char*>(address+12), 20))
            << "', '0x" << bin2hex(rsv.substr(64, 1))
            << "', '0x" << bin2hex(rsv.substr(0, 32))
            << "', '0x" << bin2hex(rsv.substr(32, 32))
            << "')" << std::endl;
}

int main(int argc, char* argv[]) {
  std::vector<std::string> priv_keys;
  unsigned char mask[32] = {0};
  unsigned char difficulty[32] = {0};
  unsigned char priv_key[32] = {0};
  unsigned char address[32] = {0};
  std::string address_bin;
  std::string mask_input, difficulty_input, address_input;

  do {
    std::cout << "beneficiary address: ";
    std::cin >> address_input;
    std::string address_bin = hex2bin(address_input);
  } while(address_bin.size() >= 32);

  for (size_t i = 0; i < address_bin.size(); i++) {
    size_t curr = 32 - address_bin.size() + i;
    address[curr] = address_bin[i];
  }

  // Input list of private keys to be used to check and generate signatures.
  std::string input_priv_key;
  std::cout << "If you have a list of mined keys, enter them one by one here" << std::endl;
  while (true) {
    std::cin >> input_priv_key;
    if (input_priv_key.size() == 64) {
      priv_keys.push_back(hex2bin(input_priv_key));
    } else {
      break;
    }
  }

  for (auto& pk : priv_keys) {
    check_and_sign(reinterpret_cast<const unsigned char*>(pk.c_str()), address);
  }

  // Input the mask, difficulty and hash of the benefactors address
  std::cout << "Input should be up to 64 HEX values. When it's less than that, the value of the "
        "mask and difficulty will be set as prefix, address as sufix"
        << std::endl;
  std::cout << "mask: ";
  std::cin >> mask_input;
  std::cout << "difficulty prefix: ";
  std::cin >> difficulty_input;

  std::string mask_bin = hex2bin(mask_input);
  std::string difficulty_bin = hex2bin(difficulty_input);

  for (size_t i = 0; i < mask_bin.size(); i++) {
    mask[i] = mask_bin[i];
  }
  for (size_t i = 0; i < difficulty_bin.size(); i++) {
    difficulty[i] = difficulty_bin[i];
  }

  while (1) {
    bool mined = mine_key(mask, difficulty, priv_key);
    if (mined) {
      check_and_sign(priv_key, address);
    }
  }

  return 0;
}
