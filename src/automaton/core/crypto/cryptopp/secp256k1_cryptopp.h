#ifndef AUTOMATON_CORE_CRYPTO_CRYPTOPP_SECP256K1_CRYPTOPP_H_
#define AUTOMATON_CORE_CRYPTO_CRYPTOPP_SECP256K1_CRYPTOPP_H_

#include "automaton/core/crypto/digital_signature.h"
#include "eccrypto.h"  // NOLINT

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

class secp256k1_cryptopp : public digital_signature {
 public:
  size_t public_key_size();
  size_t private_key_size();
  size_t signature_size();
  size_t k_size();
  bool has_deterministic_signatures();

  // Input should be byte array encoding the integer,
  // each byte representing 2 4-byte values
  void gen_public_key(const uint8_t * private_key,
                      uint8_t * public_key);


  void sign(const uint8_t * private_key,
            const uint8_t * message,
            const size_t msg_len,
            uint8_t * signature);

  void sign_deterministic(const uint8_t * private_key,
                          const uint8_t * message,
                          const size_t msg_len,
                          const uint8_t * k,
                          uint8_t * signature);

  bool verify(const uint8_t * public_key,
              const uint8_t * message,
              const size_t msg_len,
              const uint8_t * signature);
};

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton

#endif  //  AUTOMATON_CORE_CRYPTO_CRYPTOPP_SECP256K1_CRYPTOPP_H_
