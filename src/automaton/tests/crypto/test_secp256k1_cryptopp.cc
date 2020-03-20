#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

#include <string>
#include <vector>
#include "automaton/core/crypto/cryptopp/secp256k1_cryptopp.h"
#include "automaton/core/crypto/digital_signature.h"
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::secp256k1_cryptopp;
using automaton::core::crypto::digital_signature;
using automaton::core::io::bin2hex;

void decode_from_hex(std::string &encoded, std::string &decoded) {   // NOLINT
  CryptoPP::StringSource ss(encoded, true,
    new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded)));
}

TEST(secp256k1_cryptopp, gen_public_key) {
  digital_signature* tester = new secp256k1_cryptopp();
  EXPECT_NE(tester, nullptr);
  uint8_t* public_key = new uint8_t[tester->public_key_size()];
  constexpr uint32_t test_cases = 4;
  std::string test[test_cases][2] = {
    {"5F3AA3BB3129DB966915A6D341FDE4C95121B5F4CEDC3BA4ECC3DD44BA9A50BC",
     "02B4A66219F8E6E594979D8C1961BE1AA98E8384B534D54519217E0FBBE4EA608D"},
    {"77F8406C4620450C9BB233E6CC404BB23A6BF86AF3C943DF8F0710F612D7FF23",
     "0201BDDF939A6AB9ACF928A38C5973C752D8018D17E9F24D09287D2C0A2C06F852"},
    {"B33230BF39182DC6E158D686C8B614FA24D80AC6DB8CFA13465FAEDB12EDF6A4",
     "03A09C9E2F6472A1E73AC5A4A9A97B09D9391F57D3A38D1733B7CF672CFC645699"},
    {"E3B0F44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855",
     "0308E9692DD9F3CD5061167D7BFF031C76C63F5BD492909F26826FE399BDA2E1EB"}
  };
  for (uint32_t i = 0; i < test_cases; i++) {
    std::string pr_key_decoded;
    decode_from_hex(test[i][0], pr_key_decoded);
    tester->gen_public_key(reinterpret_cast<const uint8_t*>(pr_key_decoded.data()), public_key);

    std::string result(reinterpret_cast<char*>(public_key), tester->public_key_size());
    EXPECT_EQ(bin2hex(result), test[i][1]);
  }
}

TEST(secp256k1_cryptopp, sign_and_verify) {
  digital_signature* tester = new secp256k1_cryptopp();
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
