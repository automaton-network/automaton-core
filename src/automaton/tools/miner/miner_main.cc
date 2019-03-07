#include <assert.h>
#include <iomanip>
#include <iostream>
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
      std::cout << "Key mined" << std::endl;
      bin2hex(std::string(reinterpret_cast<char*>(priv_key)));
    }

    std::string rsv = sign(priv_key, address);

    std::cout << "rsv: " << bin2hex(rsv) << std::endl;
  }


  return 0;
}
