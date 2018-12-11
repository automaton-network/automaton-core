#include <string>
#include "gtest/gtest.h"
#include "automaton/core/state/dummy_state.h"
#include "automaton/core/crypto/SHA256_cryptopp.h"

TEST(dummy_state, get_node_hash) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  EXPECT_EQ(s.get_node_hash(""), "");
}

TEST(dummy_state, get_should_not_change_state) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  EXPECT_EQ(s.get("a"), "");
  EXPECT_EQ(s.get_node_hash(""), "");
}

TEST(dummy_state, commit_changes) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  s.commit_changes();
  EXPECT_EQ(s.get_node_hash(""), "O2\x4Je_2\xE8R\x8E\xDE\xA6M\xBF\xD1\x1C\xBA\x81\v\x87\x90\xE6\xE6\xE2=(\xAD:u\x98\a4"); // NOLINT
}

TEST(dummy_state, discard_changes) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), "");
}

TEST(dummy_state, insert_and_delete_expect_blank) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  s.commit_changes();
  s.set("a", "");
  s.set("b", "");
  s.set("c", "");
  s.commit_changes();
  EXPECT_EQ(s.get_node_hash(""), "");
}

TEST(dummy_state, set) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("a", "1");
  EXPECT_EQ(s.get("a"), "1");
  s.discard_changes();
  EXPECT_EQ(s.get("a"), "");
}

TEST(dummy_state, set_multiple_times) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("a", "1");
  EXPECT_EQ(s.get("a"), "1");
  s.set("a", "");
  EXPECT_EQ(s.get("a"), "");
  s.commit_changes();
  EXPECT_EQ(s.get("a"), "");
}

TEST(dummy_state, delete_node_tree) {
  SHA256_cryptopp hash;
  dummy_state s(&hash);
  s.set("aa", "1");
  s.set("aaa", "2");
  s.set("abc", "3");
  s.set("a2z", "4");
  s.commit_changes();
  s.set("a", "test");
  s.delete_node_tree("a");
  s.commit_changes();
  EXPECT_EQ(s.get_node_hash(""), "");
}
