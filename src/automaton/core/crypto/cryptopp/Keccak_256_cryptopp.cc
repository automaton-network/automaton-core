#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"

#include "cryptlib.h"  // NOLINT
#include "keccak.h"  // NOLINT

#include "automaton/core/crypto/hash_transformation.h"

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

Keccak_256_cryptopp::Keccak_256_cryptopp() {
  hash = new CryptoPP::Keccak_256;
}

void Keccak_256_cryptopp::calculate_digest(const uint8_t * input,
                                           const size_t length,
                                           uint8_t * digest) {
  hash->CalculateDigest(digest, length == 0 ? nullptr : input, length);
}

void Keccak_256_cryptopp::update(const uint8_t * input,
                                 const size_t length) {
  hash->Update(length == 0 ? nullptr : input, length);
}

void Keccak_256_cryptopp::final(uint8_t * digest) {
  hash->Final(digest);
}

void Keccak_256_cryptopp::restart() {
  hash->Restart();
}

uint32_t Keccak_256_cryptopp::digest_size() const {
  return _digest_size;
}

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton
