#ifndef AUTOMATON_CORE_CRYPTO_CRYPTOPP_RIPEMD160_CRYPTOPP_H_
#define AUTOMATON_CORE_CRYPTO_CRYPTOPP_RIPEMD160_CRYPTOPP_H_

#include "automaton/core/crypto/hash_transformation.h"

#include "cryptlib.h"  // NOLINT
#include "ripemd.h"  // NOLINT

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

class RIPEMD160_cryptopp : public hash_transformation {
 private:
  CryptoPP::RIPEMD160* hash;
 public:
  RIPEMD160_cryptopp();
  ~RIPEMD160_cryptopp();

  void calculate_digest(const uint8_t* input,
                        const size_t length,
                        uint8_t* digest);

  void update(const uint8_t* input, const size_t length);

  void final(uint8_t* digest);

  void restart();

  uint32_t digest_size() const;

 private:
  static const int _digest_size = 20;
};

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_CRYPTO_CRYPTOPP_RIPEMD160_CRYPTOPP_H_
