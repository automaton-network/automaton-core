#ifndef AUTOMATON_CORE_CRYPTO_CRYPTOPP_SHA3_256_CRYPTOPP_H_
#define AUTOMATON_CORE_CRYPTO_CRYPTOPP_SHA3_256_CRYPTOPP_H_

#include "automaton/core/crypto/hash_transformation.h"

#include "sha3.h"  // NOLINT

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

class SHA3_256_cryptopp : public hash_transformation {
 private:
  CryptoPP::SHA3_256* hash;
 public:
  SHA3_256_cryptopp();
  ~SHA3_256_cryptopp();

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

#endif  // AUTOMATON_CORE_CRYPTO_CRYPTOPP_SHA3_256_CRYPTOPP_H_
