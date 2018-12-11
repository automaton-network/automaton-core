#include "automaton/examples/crypto/basic_hash_miner.h"
#include <string>
#include <vector>
#include "automaton/core/crypto/hash_transformation.h"

using automaton::core::crypto::hash_transformation;

namespace automaton {
namespace examples {

void basic_hash_miner::next_nonce() {
  int current = nonce_lenght_ - 1;

  while (nonce_[current] == 255) {
    nonce_[current] = 0;
    current--;
  }

  nonce_[current]++;
}

bool basic_hash_miner::is_valid_next_block_hash(uint8_t* hash,
                                                int required_leading_zeros) {
  int current = 0;
  while (required_leading_zeros - 8 >= 0) {
    if (hash[current] != 0) {
      return false;
    }
    current++;
    required_leading_zeros -= 8;
  }

  if (required_leading_zeros > 0) {
    return (hash[current] & ((1 << ( 8 - required_leading_zeros)) - 1))
           == hash[current];
  } else {
    return true;
  }
}

basic_hash_miner::basic_hash_miner(hash_transformation* hash_transformation) {
  hash_transformation_ = hash_transformation;
  nonce_lenght_ = 32;
}

int basic_hash_miner::get_nonce_lenght() {
  return nonce_lenght_;
}

uint8_t* basic_hash_miner::mine(const uint8_t* block_hash,
    const int block_hash_lenght,
    const int required_leading_zeros) {
  if (required_leading_zeros > nonce_lenght_ * 8) {
    // TODO(martin) log the invalid input
    return NULL;
  }

  int digest_size = hash_transformation_ -> digest_size();
  uint8_t* next_block_hash = new uint8_t[digest_size];
  nonce_ = new uint8_t[nonce_lenght_]();

  do {
    next_nonce();
    hash_transformation_ -> restart();

    hash_transformation_ -> update(block_hash, block_hash_lenght);
    hash_transformation_ -> update(nonce_, nonce_lenght_);
    hash_transformation_ -> final(next_block_hash);
  } while (!is_valid_next_block_hash(next_block_hash, required_leading_zeros));

  return nonce_;
}

}  // namespace examples
}  // namespace automaton
