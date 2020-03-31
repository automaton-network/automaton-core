#include "automaton/core/state/state_persistent.h"
#include <stdio.h>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/io/io.h"
#include "automaton/core/storage/persistent_blobstore.h"
#include "automaton/core/storage/persistent_vector.h"
#include "gtest/gtest.h"

using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::io::bin2hex;
using automaton::core::state::state_persistent;
using automaton::core::storage::blobstore;
using automaton::core::storage::persistent_blobstore;
using automaton::core::storage::persistent_vector;

TEST(state_persistent, set_and_get) {
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
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_set_and_get");
  remove("build/mapped_vector_set_and_get");
  bs->map_file("build/mapped_file_set_and_get");
  pv->map_file("build/mapped_vector_set_and_get");
  state_persistent state(hasher, bs, pv);

  // For each node added, check if the previous nodes are still correct
  for (unsigned int i = 0; i < tests.size(); i++) {
    state.set(tests[i].first, tests[i].second);
    for (unsigned int j = 0; j <= i; j++) {
      EXPECT_EQ(state.get(tests[j].first), tests[j].second);
    }
  }
}

TEST(state_persistent, set_delete_and_get) {
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
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_set_delete_and_get");
  remove("build/mapped_vector_set_delete_and_get");
  bs->map_file("build/mapped_file_set_delete_and_get");
  pv->map_file("build/mapped_vector_set_delete_and_get");
  state_persistent state(hasher, bs, pv);
  // add all nodes
  for (unsigned int i = 0; i < tests.size(); i++) {
    state.set(tests[i].first, tests[i].second);
  }
  // delete one and check if remaining nodes are correct
  for (unsigned int i = 0; i < tests.size(); i++) {
    state.erase(tests[i].first);
    for (unsigned int j = i+1; j < tests.size(); j++) {
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

TEST(state_persistent, node_hash_add_erase) {
  std::stack<std::string> root_hashes;
  std::stack<std::string> keys;
  int32_t key_count = 1000;


  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_node_hash_add_erase");
  remove("build/mapped_vector_node_hash_add_erase");
  bs->map_file("build/mapped_file_node_hash_add_erase");
  pv->map_file("build/mapped_vector_node_hash_add_erase");
  state_persistent state(hasher, bs, pv);

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
      // throw std::domain_error("BAD " + std::to_string(i));
    }
    root_hashes.pop();
  }
}

TEST(state_persistent, insert_and_delete_expect_blank) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_insert_and_delete_expect_blank");
  remove("build/mapped_vector_insert_and_delete_expect_blank");
  bs->map_file("build/mapped_file_insert_and_delete_expect_blank");
  pv->map_file("build/mapped_vector_insert_and_delete_expect_blank");
  state_persistent state(hasher, bs, pv);

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

TEST(state_persistent, get_node_hash) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_get_node_hash");
  remove("build/mapped_vector_get_node_hash");
  bs->map_file("build/mapped_file_get_node_hash");
  pv->map_file("build/mapped_vector_get_node_hash");
  state_persistent state(hasher, bs, pv);
  EXPECT_EQ(state.get_node_hash(""), "");
}

TEST(state_persistent, commit_changes) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_commit_changes");
  remove("build/mapped_vector_commit_changes");
  bs->map_file("build/mapped_file_commit_changes");
  pv->map_file("build/mapped_vector_commit_changes");
  state_persistent state(hasher, bs, pv);

  state.set("a", "1");
  state.set("b", "2");
  state.set("c", "3");
  std::string root_hash = state.get_node_hash("");
  state.commit_changes();
  EXPECT_EQ(state.get_node_hash(""), root_hash);
}

TEST(state_persistent, discard_changes) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_discard_changes");
  remove("build/mapped_vector_discard_changes");
  bs->map_file("build/mapped_file_discard_changes");
  pv->map_file("build/mapped_vector_discard_changes");
  state_persistent state(hasher, bs, pv);
  state.set("a", "1");
  state.set("b", "2");
  state.set("c", "3");
  state.discard_changes();
  EXPECT_EQ(state.get_node_hash(""), "");
}


TEST(state_persistent, delete_node_tree) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_delete_node_tree");
  remove("build/mapped_vector_delete_node_tree");
  bs->map_file("build/mapped_file_delete_node_tree");
  pv->map_file("build/mapped_vector_delete_node_tree");
  state_persistent state(hasher, bs, pv);
  state.set("aa", "1");
  state.set("aaa", "2");
  state.set("abc", "3");
  state.set("a2z", "4");
  state.set("a", "test");
  state.delete_node_tree("a");
  EXPECT_EQ(state.get_node_hash(""), "");
}
// This function tests if free locations are used correclty when combined
// with delete_node_tree, commit_changes, discard_changestate.
// Deleted nodes should be backed up only when we create new node at
// their location.
TEST(state_persistent, delete_node_tree_plus_commit_discard_free_backup_add_node) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_delete_node_tree_plus_commit_discard_free_backup_add_node");
  remove("build/mapped_vector_delete_node_tree_plus_commit_discard_free_backup_add_node");
  bs->map_file("build/mapped_file_delete_node_tree_plus_commit_discard_free_backup_add_node");
  pv->map_file("build/mapped_vector_delete_node_tree_plus_commit_discard_free_backup_add_node");
  state_persistent state(hasher, bs, pv);
  state.set("aa", "1");
  state.set("aaa", "2");
  state.set("abc", "3");
  state.set("a2z", "4");
  state.set("a", "test");
  state.set("not_a_path", "lets have few more paths");
  state.set("unrelated_path", "just few more paths unrelated to 1");
  state.set("branch_two", "other branch to play with");
  state.set("branch_mewtwo", "branch to play with");
  state.set("branch_mew", "to play with");
  state.set("branch_arrow", "play with");
  state.set("branch_two_but_longer", "with");
  state.set("branch", "Oh no, branches to paly with are gone");
  state.commit_changes();
  std::string hash_before_discard = state.get_node_hash("");
  state.delete_node_tree("a");
  state.discard_changes();
  EXPECT_EQ(state.get_node_hash(""), hash_before_discard);

  state.delete_node_tree("a");
  state.set("evil_node", "lets overwrite empty locations");
  state.set("a_starting_node", "lets overwrite empty locations");
  state.set("aaa", "I will act like I am legit node to avoid getting discarded");
  state.discard_changes();
  EXPECT_EQ(state.get_node_hash(""), hash_before_discard);

  state.set("branch", "I replace the original");
  state.delete_node_tree("branch");
  state.discard_changes();
  EXPECT_EQ(state.get_node_hash(""), hash_before_discard);

  state.set("brach_mew", "ancestor to be deleted");
  state.erase("branch_mewtwo");
  state.set("branch_mewtwo", "I'm back, but different");
  state.delete_node_tree("branch");
  state.discard_changes();
  EXPECT_EQ(state.get_node_hash(""), hash_before_discard);
}


TEST(dummy_state, using_deleted_locations) {
  hash_transformation* hasher = new SHA256_cryptopp();
  persistent_blobstore* bs = new persistent_blobstore();
  persistent_vector<state_persistent::node>* pv = new persistent_vector<state_persistent::node>();
  remove("build/mapped_file_using_deleted_locations");
  remove("build/mapped_vector_using_deleted_locations");
  bs->map_file("build/mapped_file_using_deleted_locations");
  pv->map_file("build/mapped_vector_using_deleted_locations");
  state_persistent state(hasher, bs, pv);

  state.set("a", "1");
  state.set("b", "2");
  state.set("c", "3");
  state.set("d", "4");
  EXPECT_EQ(state.size(), 5U);
  state.commit_changes();
  EXPECT_EQ(state.size(), 5U);

  state.erase("a");
  EXPECT_EQ(state.size(), 5U);
  state.commit_changes();
  EXPECT_EQ(state.size(), 4U);

  state.erase("b");
  state.discard_changes();
  EXPECT_EQ(state.size(), 4U);

  state.set("a", "1");
  state.erase("b");
  EXPECT_EQ(state.size(), 5U);
  state.discard_changes();
  EXPECT_EQ(state.size(), 4U);

  state.erase("b");
  state.set("a", "1");
  EXPECT_EQ(state.size(), 4U);
  state.discard_changes();
  EXPECT_EQ(state.size(), 4U);

  state.set("a", "1");
  EXPECT_EQ(state.size(), 5U);
  state.erase("b");
  state.set("x", "2");
  EXPECT_EQ(state.size(), 5U);
  state.discard_changes();
  EXPECT_EQ(state.size(), 4U);

  state.erase("b");
  state.commit_changes();
  EXPECT_EQ(state.size(), 3U);
  state.set("e", "1");
  EXPECT_EQ(state.size(), 4U);
}
