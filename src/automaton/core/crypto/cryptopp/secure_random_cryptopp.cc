#include <stdint.h>

#include "automaton/core/crypto/cryptopp/secure_random_cryptopp.h"

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

bool secure_random_cryptopp::bit() {
  return prng.GenerateBit();
}

void secure_random_cryptopp::block(uint8_t * output, size_t size) {
  prng.GenerateBlock(output, size);
}

uint8_t secure_random_cryptopp::byte() {
  return prng.GenerateByte();
}

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton
