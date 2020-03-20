#ifndef AUTOMATON_CORE_CRYPTO_CRYPTOPP_KECCAK_256_CRYPTOPP_H_
#define AUTOMATON_CORE_CRYPTO_CRYPTOPP_KECCAK_256_CRYPTOPP_H_

#include <cryptopp/cryptlib.h>
#include <cryptopp/keccak.h>

#include "automaton/core/crypto/hash_transformation.h"

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

class Keccak_256_cryptopp : public hash_transformation {
 private:
  CryptoPP::Keccak_256 * hash;
 public:
  Keccak_256_cryptopp();
  ~Keccak_256_cryptopp();

  void calculate_digest(const uint8_t* input,
    const size_t length,
    uint8_t* digest);

  void update(const uint8_t* input, const size_t length);

  void final(uint8_t* digest);

  void restart();

  uint32_t digest_size() const;

 private:
  static const int _digest_size = 32;
};

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_CRYPTO_CRYPTOPP_KECCAK_256_CRYPTOPP_H_
