#include "automaton/core/crypto/hash_transformation.h"
#include "gtest/gtest.h"

using automaton::core::crypto::hash_transformation;

template<uint8_t C>
class dummy_hash : public hash_transformation {
 public:
  void update(const uint8_t * input, const size_t length) {}

  void final(uint8_t * digest) {
    digest[0] = C;
  }

  void restart() {}

  uint32_t digest_size() const {
    return 1;
  }
};

const char* DUMMY1 = "dummy1";
const char* DUMMY2 = "dummy2";
const char* DUMMY3 = "dummy3";

const uint8_t * TEST1 = (const uint8_t*)"abc";
const uint8_t * TEST2 = (const uint8_t*)"test";
