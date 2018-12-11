#include "automaton/core/crypto/hash_transformation.h"

#include <string>

#include "automaton/core/data/msg.h"
#include "automaton/core/io/io.h"

using automaton::core::data::msg;

namespace automaton {
namespace core {
namespace crypto {

void hash_transformation::calculate_digest(const uint8_t * input,
                                           const size_t length,
                                           uint8_t * digest) {
  update(input, length);
  final(digest);
}

}  // namespace crypto
}  // namespace core
}  // namespace automaton
