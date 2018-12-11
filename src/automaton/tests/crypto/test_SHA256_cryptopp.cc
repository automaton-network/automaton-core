#include <string>
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"
#include "cryptlib.h"  // NOLINT
#include "sha.h"  // NOLINT
#include "gtest/gtest.h"  // NOLINT
#include "hex.h"  // NOLINT
#include "filters.h"  // NOLINT

using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;

TEST(SHA256_cryptopp, calculate_digest) {
  SHA256_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  constexpr uint32_t test_cases = 6;
  std::string long_a_string(1000000, 'a');

  std::string test[test_cases][2] = {
    {"a", "CA978112CA1BBDCAFAC231B39A23DC4DA786EFF8147C4E72B9807785AFEE48BB"},
    // NOLINTNEXTLINE
    {"abc", "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"},
    {"", "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
     "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1"},
    // NOLINTNEXTLINE
    {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
     "CF5B16A778AF8380036CE59E7B0492370B249B11E8F07A51AFAC45037AFEE9D1"},
    {long_a_string,
      "CDC76E5C9914FB9281A1C7E284D73E67F1809A48A497200E046D39CCC7112CD0"}
  };

  for (uint32_t i = 0; i < test_cases; i++) {
    hasher.calculate_digest(reinterpret_cast<const uint8_t*>(test[i][0].data()),
        test[i][0].length(), digest);
    std::string result(reinterpret_cast<char*>(digest), digest_size);
    EXPECT_EQ(bin2hex(result), test[i][1]);
  }

  delete[] digest;
}

TEST(SHA256_cryptopp, update_and_finish) {
  SHA256_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  std::string test_input(
      "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");
  const uint8_t* p_test_input = reinterpret_cast<const uint8_t*>(test_input.data());
  size_t len = test_input.length();

  for (uint32_t i = 0; i <  16777216; i++) {
    hasher.update(p_test_input, len);
  }
  hasher.final(digest);
  std::string result(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result), "50E72A0E26442FE2552DC3938AC58658228C0CBFB1D2CA872AE435266FCD055E");

  // Try to hash a new string to see if everything restarted as intended
  const uint8_t* a = reinterpret_cast<const uint8_t*>("a");
  const uint8_t* b = reinterpret_cast<const uint8_t*>("b");
  const uint8_t* c = reinterpret_cast<const uint8_t*>("c");
  hasher.update(a, 1);
  hasher.update(b, 1);
  hasher.update(c, 1);
  hasher.final(digest);
  std::string result2(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result2), "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD");

  delete[] digest;
}

TEST(SHA256_cryptopp, digest_size) {
  SHA256_cryptopp hasher;
  EXPECT_EQ(hasher.digest_size(), CryptoPP::SHA256::DIGESTSIZE);
}
