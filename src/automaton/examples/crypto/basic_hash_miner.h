#ifndef AUTOMATON_EXAMPLES_CRYPTO_BASIC_HASH_MINER_H_
#define AUTOMATON_EXAMPLES_CRYPTO_BASIC_HASH_MINER_H_

#include "automaton/core/crypto/hash_transformation.h"

namespace automaton {
namespace examples {

class basic_hash_miner {
 public:
  explicit basic_hash_miner(core::crypto::hash_transformation* hash_transformation);

  int get_nonce_lenght();
  uint8_t* mine(const uint8_t* block_hash,
                const int block_hash_lenght,
                const int required_leading_zeros);

 private:
  int nonce_lenght_;
  core::crypto::hash_transformation* hash_transformation_;
  uint8_t* nonce_;

  void next_nonce();
  bool is_valid_next_block_hash(uint8_t* hash,
                                int required_leading_zeros);
};

}  // namespace examples
}  // namespace automaton

#endif  // AUTOMATON_EXAMPLES_CRYPTO_BASIC_HASH_MINER_H_
