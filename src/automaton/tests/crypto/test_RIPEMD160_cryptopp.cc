#include <cryptopp/cryptlib.h>
#include <cryptopp/integer.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

#include <string>

#include "automaton/core/crypto/cryptopp/RIPEMD160_cryptopp.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::RIPEMD160_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;

TEST(RIPEMD160_cryptopp, calculate_digest) {
  RIPEMD160_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  constexpr uint32_t test_cases = 9;
  std::string long_a_string(1000000, 'a');

  std::string test[test_cases][2] = {
    {"", "9C1185A5C5E9FC54612808977EE8F548B2258D31"},
    {"a", "0BDC9D2D256B3EE9DAAE347BE6F4DC835A467FFE"},
    {"abc", "8EB208F7E05D987A9B044A8E98C6B087F15A0BFC"},
    {"message digest", "5D0689EF49D2FAE572B881B123A85FFA21595F36"},
    {"abcdefghijklmnopqrstuvwxyz", "F71C27109C692C1B56BBDCEB5B9D2865B3708DBC"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      "12A053384A9C0C88E405A06C27DCF49ADA62EB2B"},
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
      "B0E20B6E3116640286ED3A87A5713079B21F5189"},
    {long_a_string, "52783243C1697BDBE16D37F97F68F08325DC1528"},
      // NOLINTNEXTLINE
    {"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
      "9B752E45573D4B39F4DBD3323CAB82BF63326BFB"}
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
  RIPEMD160_cryptopp hasher;
  size_t digest_size = hasher.digest_size();
  uint8_t* digest = new uint8_t[digest_size];
  std::string test_input("a");
  const uint8_t* p_test_input = reinterpret_cast<const uint8_t*>(test_input.data());
  size_t len = test_input.length();

  for (uint32_t i = 0; i <  1000000; i++) {
    hasher.update(p_test_input, len);
  }
  hasher.final(digest);
  std::string result(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result), "52783243C1697BDBE16D37F97F68F08325DC1528");

  // Try to hash a new string to see if everything restarted as intended
  const uint8_t* a = reinterpret_cast<const uint8_t*>("a");
  const uint8_t* b = reinterpret_cast<const uint8_t*>("b");
  const uint8_t* c = reinterpret_cast<const uint8_t*>("c");
  hasher.update(a, 1);
  hasher.update(b, 1);
  hasher.update(c, 1);
  hasher.final(digest);
  std::string result2(reinterpret_cast<char*>(digest), digest_size);
  EXPECT_EQ(bin2hex(result2), "8EB208F7E05D987A9B044A8E98C6B087F15A0BFC");
  delete[] digest;
}

TEST(SHA256_cryptopp, digest_size) {
  RIPEMD160_cryptopp hasher;
  EXPECT_EQ(hasher.digest_size(), CryptoPP::RIPEMD160::DIGESTSIZE);
}
