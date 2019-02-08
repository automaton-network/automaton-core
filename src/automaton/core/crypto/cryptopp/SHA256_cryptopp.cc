#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"

#include "cryptlib.h"  // NOLINT
#include "sha.h"  // NOLINT

#include "automaton/core/crypto/hash_transformation.h"

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

SHA256_cryptopp::SHA256_cryptopp() {
  hash = new CryptoPP::SHA256;
}

void SHA256_cryptopp::calculate_digest(const uint8_t * input,
                                      const size_t length,
                                      uint8_t * digest) {
  hash->CalculateDigest(digest, length == 0 ? nullptr : input, length);
}

void SHA256_cryptopp::update(const uint8_t * input,
                             const size_t length) {
  hash->Update(length == 0 ? nullptr : input, length);
}

void SHA256_cryptopp::final(uint8_t * digest) {
  hash->Final(digest);
}

void SHA256_cryptopp::restart() {
  hash->Restart();
}

uint32_t SHA256_cryptopp::digest_size() const {
  return _digest_size;
}

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton
