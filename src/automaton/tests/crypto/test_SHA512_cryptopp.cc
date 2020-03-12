#include <cryptopp/cryptlib.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

#include <string>
#include "automaton/core/crypto/cryptopp/SHA512_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::SHA512_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;

TEST(SHA512_cryptopp, calculate_digest) {
  SHA512_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  constexpr uint32_t test_cases = 6;
  std::string long_a_string(1000000, 'a');

  std::string test[test_cases][2] = {
    {"a",
     "1F40FC92DA241694750979EE6CF582F2D5D7D28E18335DE05ABC54D0560E0F53"
     "02860C652BF08D560252AA5E74210546F369FBBBCE8C12CFC7957B2652FE9A75"},
    {"abc",
     "DDAF35A193617ABACC417349AE20413112E6FA4E89A97EA20A9EEEE64B55D39A"
     "2192992A274FC1A836BA3C23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F"},
    {"",
     "CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715DC83F4A921D36CE9CE"
     "47D0D13C5D85F2B0FF8318D2877EEC2F63B931BD47417A81A538327AF927DA3E"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
     "204A8FC6DDA82F0A0CED7BEB8E08A41657C16EF468B228A8279BE331A703C335"
     "96FD15C13B1B07F9AA1D3BEA57789CA031AD85C7A71DD70354EC631238CA3445"},
    {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
     "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
     "8E959B75DAE313DA8CF4F72814FC143F8F7779C6EB9F7FA17299AEADB6889018"
     "501D289E4900F7E4331B99DEC4B5433AC7D329EEB6DD26545E96E55B874BE909"},
    {long_a_string,
      "E718483D0CE769644E2E42C7BC15B4638E1F98B13B2044285632A803AFA973EB"
      "DE0FF244877EA60A4CB0432CE577C31BEB009C5C2C49AA2E4EADB217AD8CC09B"}
  };

  for (uint32_t i = 0; i < test_cases; i++) {
    hasher.calculate_digest(reinterpret_cast<const uint8_t*>(test[i][0].data()),
        test[i][0].length(), digest);
    std::string result(reinterpret_cast<char*>(digest), digest_size);
    EXPECT_EQ(bin2hex(result), test[i][1]);
  }

  delete[] digest;
}

TEST(SHA512_cryptopp, update_and_finish) {
  SHA512_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  std::string test_input(
      "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");
  const std::string EXP1 =
      "B47C933421EA2DB149AD6E10FCE6C7F93D0752380180FFD7F4629A712134831D"
      "77BE6091B819ED352C2967A2E2D4FA5050723C9630691F1A05A7281DBE6C1086";
  const uint8_t* p_test_input = reinterpret_cast<const uint8_t*>(test_input.data());
  size_t len = test_input.length();

  for (uint32_t i = 0; i <  16777216; i++) {
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
      "DDAF35A193617ABACC417349AE20413112E6FA4E89A97EA20A9EEEE64B55D39A"
      "2192992A274FC1A836BA3C23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F";
  hasher.update(a, 1);
  hasher.update(b, 1);
  hasher.update(c, 1);
  hasher.final(digest);

  std::string result2(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result2), EXP2);

  delete[] digest;
}

TEST(SHA512_cryptopp, digest_size) {
  SHA512_cryptopp hasher;
  EXPECT_EQ(hasher.digest_size(), CryptoPP::SHA512::DIGESTSIZE);
}
