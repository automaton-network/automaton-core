#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/io/io.h"
#include "automaton/core/script/engine.h"

#include "gtest/gtest.h"
#include "cryptlib.h"  // NOLINT
#include "hex.h"  // NOLINT

using automaton::core::data::protobuf::protobuf_factory;

namespace automaton {
namespace core {

class test_script : public ::testing::Test {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

struct hash_test {
  const char* hash_function;
  const char* input;
  const char* output;
};

TEST_F(test_script, module_registration) {
  hash_test tests[] = {
    {"keccak256", "", "C5D2460186F7233C927E7DB2DCC703C0E500B653CA82273B7BFAD8045D85A470"},
    {"keccak256", "abc", "4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45"},
    {"ripemd160", "", "9C1185A5C5E9FC54612808977EE8F548B2258D31"},
    {"sha256", "", "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"},
    {"sha3", "", "A7FFC6F8BF1ED76651C14756A061D662F580FF4DE43B49FA82D80A4B80F8434A"},
    {"sha512", "",
        "CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715DC83F4A921D36CE9CE"
        "47D0D13C5D85F2B0FF8318D2877EEC2F63B931BD47417A81A538327AF927DA3E"},
  };

  protobuf_factory data_factory;
  script::engine lua(data_factory);
  lua.bind_core();

  for (auto test : tests) {
    auto result = lua[test.hash_function](test.input);
    EXPECT_EQ(io::bin2hex(result), test.output);
  }
}

}  // namespace core
}  // namespace automaton
