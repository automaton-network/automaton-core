#include <string>
#include "automaton/tools/miner/miner.h"
#include "automaton/core/io/io.h"
#include "secp256k1/include/secp256k1_recovery.h"
#include "secp256k1/include/secp256k1.h"
#include "secp256k1/src/hash_impl.h"
#include "secp256k1/src/hash.h"
#include "gtest/gtest.h"  // NOLINT


using automaton::tools::miner::mine_key;

TEST(miner, generate_valid_key) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  std::string pub_key_after_mask(32, '0');

  unsigned char mask[32] = { 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                             0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                             0xFF, 0xFF };
  unsigned char difficulty[32] = { 0x80, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00 };

  unsigned char priv_key[32];

  bool success = mine_key(mask, difficulty, priv_key);
  EXPECT_EQ(success, true);

  secp256k1_ec_pubkey_create(context, pubkey, priv_key);

  unsigned char pub_key_serialized[65];
  size_t outLen = 65;
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);

  std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), 65);

  for (int i = 0; i < 32; i++) {
      pub_key_after_mask[i] = pub_key_uncompressed[i+1] ^ mask[i];
  }

  EXPECT_EQ(memcmp(difficulty, pub_key_after_mask.data(), 32), -1);

}
