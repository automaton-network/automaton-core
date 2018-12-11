#include <string>
#include <vector>
#include "automaton/core/crypto/digital_signature.h"
#include "automaton/core/crypto/ed25519_orlp/ed25519_orlp.h"
#include "gtest/gtest.h"
#include "hex.h"  // NOLINT
#include "filters.h"  // NOLINT

using automaton::core::crypto::digital_signature;
using automaton::core::crypto::ed25519_orlp::ed25519_orlp;

void decode_from_hex(std::string &encoded, std::string &decoded) {   // NOLINT
  CryptoPP::StringSource ss(encoded, true,
    new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded)));
}

TEST(secp256k1_cryptopp, gen_public_key) {
}

TEST(secp256k1_cryptopp, sign_and_verify) {
  digital_signature * tester = new ed25519_orlp;
  EXPECT_NE(tester, nullptr);
  uint8_t* public_key = new uint8_t[tester->public_key_size()];
  uint8_t* signature = new uint8_t[tester->signature_size()];
  std::vector<std::string> test_key = {
    "5f3aa3bb3129db966915a6d341fde4c95121b5f4cedc3ba4ecc3dd44ba9a50bc",
    "77f8406c4620450c9bb233e6cc404bb23a6bf86af3c943df8f0710f612d7ff23",
    "b33230bf39182dc6e158d686c8b614fa24d80ac6db8cfa13465faedb12edf6a4",
    "e3b0f44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  };
  std::vector<std::string> test_hash = {
    "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD",
    "HELLO, HELLO, HELLO",
    "We could have been friends",
    "Final Space",
  };
  for (uint32_t i = 0; i < test_key.size(); i++) {
    std::string pr_key_decoded;
    decode_from_hex(test_key[i], pr_key_decoded);
    tester->gen_public_key(reinterpret_cast<const uint8_t*>(pr_key_decoded.data()), public_key);
    for (uint32_t j = 0; j < test_hash.size(); j++) {
      tester->sign(reinterpret_cast<const uint8_t*>(pr_key_decoded.data()),
                   reinterpret_cast<const uint8_t*>(test_hash[j].data()),
                   test_hash[j].length(),
                   signature);
      EXPECT_EQ(tester->verify(public_key,
                               reinterpret_cast<const uint8_t*>(test_hash[j].data()),
                               test_hash[j].length(),
                               signature), true);
    }
  }
}
TEST(secp256k1_cryptopp, verify) {
}
TEST(secp256k1_cryptopp, check_return_sizes) {
}
