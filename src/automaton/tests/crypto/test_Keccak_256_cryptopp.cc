#include <cryptopp/cryptlib.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

#include <string>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"

#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;

TEST(keccak_256_cryptopp, calculate_digest) {
  Keccak_256_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  constexpr uint32_t test_cases = 6;

  std::string test[test_cases][2] = {
    {"a",
     "3AC225168DF54212A25C1C01FD35BEBFEA408FDAC2E31DDD6F80A4BBF9A5F1CB"},
    {"abc",
     "4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45"},
    {"",
     "C5D2460186F7233C927E7DB2DCC703C0E500B653CA82273B7BFAD8045D85A470"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
     "45D3B367A6904E6E8D502EE04999A7C27647F91FA845D456525FD352AE3D7371"},
    {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
     "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
     "F519747ED599024F3882238E5AB43960132572B7345FBEB9A90769DAFD21AD67"},
    {"testing",
      "5F16F4C7F149AC4F9510D9CF8CF384038AD348B3BCDC01915F95DE12DF9D1B02"}
  };

  for (uint32_t i = 0; i < test_cases; i++) {
    hasher.calculate_digest(reinterpret_cast<const uint8_t*>(test[i][0].data()),
        test[i][0].length(), digest);
    std::string result(reinterpret_cast<char*>(digest), digest_size);
    EXPECT_EQ(bin2hex(result), test[i][1]);
  }

  delete[] digest;
}

TEST(keccak_256_cryptopp, update_and_finish) {
  Keccak_256_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  std::string test_input(
      "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");
  const std::string EXP1 =
      "C8A625720D2C6221C09DB8A33A63FB936E628A0C10195768A206E7AD8D1E54DE";
  const uint8_t* p_test_input = reinterpret_cast<const uint8_t*>(test_input.data());
  size_t len = test_input.length();

  for (uint32_t i = 0; i < 10; i++) {
    hasher.update(p_test_input, len);
  }
  hasher.final(digest);

  std::string result(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result), EXP1);

  // Try to hash a new string to see if everything restarted as intended
  const uint8_t* a = reinterpret_cast<const uint8_t*>("a");
  const uint8_t* b = reinterpret_cast<const uint8_t*>("b");
  const uint8_t* c = reinterpret_cast<const uint8_t*>("c");
  const std::string EXP2 =
      "4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45";
  hasher.update(a, 1);
  hasher.update(b, 1);
  hasher.update(c, 1);
  hasher.final(digest);

  std::string result2(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result2), EXP2);

  delete[] digest;
}

TEST(keccak_256_cryptopp, digest_size) {
  Keccak_256_cryptopp hasher;
  EXPECT_EQ(hasher.digest_size(), CryptoPP::Keccak_256::DIGESTSIZE);
}
