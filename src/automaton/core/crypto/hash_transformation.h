#ifndef AUTOMATON_CORE_CRYPTO_HASH_TRANSFORMATION_H__
#define AUTOMATON_CORE_CRYPTO_HASH_TRANSFORMATION_H__

#include <iostream>
#include <map>
#include <string>

namespace automaton {
namespace core {
namespace crypto {

// Base class for hash functions
class hash_transformation {
 public:
  // Updates the hash with additional input and computes the hash of the current
  // message.
  // Precondition digest_size == digest in bytes.
  // IN:  input:    the input as a buffer.
  //      lenght:   the size of the buffer, in bytes.
  // OUT: digest:   a pointer to the buffer to recive the hash.
  virtual void calculate_digest(const uint8_t * input,
                                const size_t length,
                                uint8_t * digest);

  // Update a hash with additional input.
  // IN:  input:    the additional input as a buffer.
  //      lenght:   the size of the buffer, in bytes .
  virtual void update(const uint8_t * input, const size_t length) = 0;

  // Computes the hash of the current message.
  // Precondition digest_size == digest in bytes.
  // OUT:  digest:  a pointer to the buffer to receive the hash.
  virtual void final(uint8_t * digest) = 0;

  // Restarts the hash, by discarding and re-initializing the state.
  virtual void restart() = 0;

  // Provides the digest size of the hash.
  virtual uint32_t digest_size() const = 0;

  virtual ~hash_transformation() {}
};

}  // namespace crypto
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_CRYPTO_HASH_TRANSFORMATION_H__
