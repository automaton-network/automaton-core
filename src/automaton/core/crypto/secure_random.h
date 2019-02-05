#ifndef AUTOMATON_CORE_CRYPTO_SECURE_RANDOM_H__
#define AUTOMATON_CORE_CRYPTO_SECURE_RANDOM_H__

#include <cstdint>
#include <map>
#include <string>

namespace automaton {
namespace core {
namespace crypto {

// Class used for getting cryptographically secure random
class secure_random {
 public:
  // Generate random bit
  virtual bool bit() = 0;

  // Generate random array of bytes.
  // Instantiate a class using the registered function in the factory.
  // IN:  size:       The size of the memblock block in bytes.
  // OUT: memblock:   Memory location to save the random bytes.
  virtual void block(uint8_t * memblock, size_t size) = 0;

  // Generate random array of byte
  virtual uint8_t byte() = 0;

  virtual ~secure_random() {}
};

}  // namespace crypto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_CRYPTO_SECURE_RANDOM_H__
