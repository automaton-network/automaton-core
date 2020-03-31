#include <string>
#include <vector>
#include <utility>
#include <stack>
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/io/io.h"
#include "automaton/core/state/state_impl.h"
#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;
using automaton::core::state::state_impl;

TEST(state_impl, set_and_get) {
  std::vector<std::pair<std::string, std::string> > tests;
  tests.push_back(std::make_pair("test", "1"));
  tests.push_back(std::make_pair("tester", "2"));
  tests.push_back(std::make_pair("ala", "ala"));
  tests.push_back(std::make_pair("alabala", "alabala"));
  tests.push_back(std::make_pair("testing", "3"));
  tests.push_back(std::make_pair("telting", "3.1"));
  tests.push_back(std::make_pair("travel", "4"));
  tests.push_back(std::make_pair("tramway", "5"));
  tests.push_back(std::make_pair("tram", "6"));
  tests.push_back(std::make_pair("tramva", "7"));

  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl state(hasher);

  // For each node added, check if the previous nodes are still correct
  for (uint32_t i = 0; i < tests.size(); i++) {
    state.set(tests[i].first, tests[i].second);
      for (uint32_t j = 0; j <= i; j++) {
        EXPECT_EQ(state.get(tests[j].first), tests[j].second);
      }
  }
}

TEST(state_impl, set_delete_and_get) {
  std::vector<std::pair<std::string, std::string> > tests;
  tests.push_back(std::make_pair("test", "1"));
  tests.push_back(std::make_pair("tester", "2"));
  tests.push_back(std::make_pair("ala", "ala"));
  tests.push_back(std::make_pair("alabala", "alabala"));
  tests.push_back(std::make_pair("testing", "3"));
  tests.push_back(std::make_pair("telting", "3.1"));
  tests.push_back(std::make_pair("travel", "4"));
  tests.push_back(std::make_pair("tramway", "5"));
  tests.push_back(std::make_pair("tram", "6"));
  tests.push_back(std::make_pair("tramva", "7"));

  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl state(hasher);
  // add all nodes
  for (uint32_t i = 0; i < tests.size(); i++) {
    state.set(tests[i].first, tests[i].second);
  }
  // delete one and check if remaining nodes are currect
  for (uint32_t i = 0; i < tests.size(); i++) {
    state.erase(tests[i].first);
    for (uint32_t j = i+1; j < tests.size(); j++) {
      EXPECT_EQ(state.get(tests[j].first), tests[j].second);
    }
  }
}

std::string hash_key(int i) {
  uint8_t digest32[32];
  hash_transformation* hasher = new SHA256_cryptopp();
  std::string data = std::to_string(i);
  hasher->calculate_digest((const uint8_t*) data.data(), data.length(),
      digest32);
  return std::string(reinterpret_cast<char*>(digest32), 16 + i % 16);
}

TEST(state_impl, node_hash_add_erase) {
  std::stack<std::string> root_hashes;
  std::stack<std::string> keys;
  int32_t key_count = 100000;

  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl state(hasher);

  // Add keys/values to the state and add the root hash into a stack.
  for (int32_t i = 0; i < key_count; ++i) {
    root_hashes.push(state.get_node_hash(""));
    std::string key = hash_key(i);
    std::string data = std::to_string(i);
    keys.push(key);

    state.set(keys.top(), data);
    EXPECT_EQ(data, state.get(keys.top()));

    if (i % (key_count/10)) {
      continue;
    }
    // Integrity check for all prior key/values.
    std::cout << i << std::endl;
    for (int32_t j = 0; j <= i; j++) {
      std::string data2 = std::to_string(j);
      std::string key2 = hash_key(j);

      if (data2 != state.get(key2)) {
        std::cout << "Setting " << i << " fails at " << j << std::endl;
        std::cout << "Setting key " << bin2hex(keys.top())
          << " fails " << bin2hex(key2) << std::endl;
        throw "!!!";
      }
    }
  }

  // Erase the keys in reverse order and check if root hash is the saved one
  // for the same trie state

  for (int32_t i = 0; i < key_count; i++) {
    state.erase(keys.top());

    // Integrity check for all prior key/values.
    if (i % (key_count/10) == 0) {
      std::cout << i << std::endl;
      for (int32_t j = 0; j < key_count - i - 1; j++) {
        std::string data = std::to_string(j);
        std::string key = hash_key(j);

        if (data != state.get(key)) {
          std::cout << "Deleting " << (key_count - i) << " fails at "
              << j << std::endl;
          std::cout << "Deleting key " << bin2hex(keys.top())
            << " fails " << bin2hex(key) << std::endl;
          throw std::domain_error("!!!");
        }
      }
    }

    keys.pop();

    if (i % 1000 == 0) {
      std::cout << "Passed " << i << " deletions in reverse order.\n";
    }
    EXPECT_EQ(state.get_node_hash(""), root_hashes.top());
    if (state.get_node_hash("") != root_hashes.top()) {
      throw std::domain_error("BAD " + std::to_string(i));
    }
    root_hashes.pop();
  }
}

TEST(state_impl, insert_and_delete_expect_blank) {
  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl state(hasher);

  state.set("a", "1");
  state.set("b", "2");
  state.set("c", "3");
  state.commit_changes();
  state.set("a", "");
  state.set("b", "");
  state.set("c", "");
  state.commit_changes();
  EXPECT_EQ(state.get(""), "");
}


TEST(state_impl, get_node_hash) {
  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl s(hasher);
  EXPECT_EQ(s.get_node_hash(""), "");
}

TEST(state_impl, commit_changes) {
  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl s(hasher);
  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  std::string root_hash = s.get_node_hash("");
  s.commit_changes();
  EXPECT_EQ(s.get_node_hash(""), root_hash);
}

TEST(state_impl, discard_changes) {
  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl s(hasher);
  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), "");
}


TEST(state_impl, delete_node_tree) {
  SHA256_cryptopp hash;
  state_impl s(&hash);
  s.set("aa", "1");
  s.set("aaa", "2");
  s.set("abc", "3");
  s.set("a2z", "4");
  s.set("a", "test");
  s.delete_node_tree("a");
  EXPECT_EQ(s.get_node_hash(""), "");
}
// This function tests if free locations are used correclty when combined
// with delete_node_tree, commit_changes, discard_changes.
// Deleted nodes should be backed up only when we create new node at
// their location.
TEST(state_impl, delete_node_tree_plus_commit_discard_free_backup_add_node) {
  SHA256_cryptopp hash;
  state_impl s(&hash);
  s.set("aa", "1");
  s.set("aaa", "2");
  s.set("abc", "3");
  s.set("a2z", "4");
  s.set("a", "test");
  s.set("not_a_path", "lets have few more paths");
  s.set("unrelated_path", "just few more paths unrelated to 1");
  s.set("branch_two", "other branch to play with");
  s.set("branch_mewtwo", "branch to play with");
  s.set("branch_mew", "to play with");
  s.set("branch_arrow", "play with");
  s.set("branch_two_but_longer", "with");
  s.set("branch", "Oh no, branches to paly with are gone");
  s.commit_changes();
  std::string hash_before_discard = s.get_node_hash("");
  s.delete_node_tree("a");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), hash_before_discard);

  s.delete_node_tree("a");
  s.set("evil_node", "lets overwrite empty locations");
  s.set("a_starting_node", "lets overwrite empty locations");
  s.set("aaa", "I will act like I am legit node to avoid getting discarded");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), hash_before_discard);

  s.set("branch", "I replace the original");
  s.delete_node_tree("branch");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), hash_before_discard);

  s.set("brach_mew", "ancestor to be deleted");
  s.erase("branch_mewtwo");
  s.set("branch_mewtwo", "I'm back, but different");
  s.delete_node_tree("branch");
  s.discard_changes();
  EXPECT_EQ(s.get_node_hash(""), hash_before_discard);
}


TEST(dummy_state, using_deleted_locations) {
  SHA256_cryptopp hash;
  state_impl s(&hash);

  s.set("a", "1");
  s.set("b", "2");
  s.set("c", "3");
  s.set("d", "4");
  EXPECT_EQ(s.size(), 5U);
  s.commit_changes();
  EXPECT_EQ(s.size(), 5U);

  s.erase("a");
  EXPECT_EQ(s.size(), 5U);
  s.commit_changes();
  EXPECT_EQ(s.size(), 4U);

  s.erase("b");
  s.discard_changes();
  EXPECT_EQ(s.size(), 4U);

  s.set("a", "1");
  s.erase("b");
  EXPECT_EQ(s.size(), 5U);
  s.discard_changes();
  EXPECT_EQ(s.size(), 4U);

  s.erase("b");
  s.set("a", "1");
  EXPECT_EQ(s.size(), 4U);
  s.discard_changes();
  EXPECT_EQ(s.size(), 4U);

  s.set("a", "1");
  EXPECT_EQ(s.size(), 5U);
  s.erase("b");
  s.set("x", "2");
  EXPECT_EQ(s.size(), 5U);
  s.discard_changes();
  EXPECT_EQ(s.size(), 4U);

  s.erase("b");
  s.commit_changes();
  EXPECT_EQ(s.size(), 3U);
  s.set("e", "1");
  EXPECT_EQ(s.size(), 4U);
}
