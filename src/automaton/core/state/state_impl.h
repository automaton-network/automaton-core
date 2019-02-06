#ifndef AUTOMATON_CORE_STATE_STATE_IMPL_H_
#define AUTOMATON_CORE_STATE_STATE_IMPL_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/state/state.h"

namespace automaton {
namespace core {
namespace state {

class state_impl : public state {
 public:
  explicit state_impl(crypto::hash_transformation* hasher);

  // Get the value at given path. Empty string if no value is set or
  // there is no node at the given path
  std::string get(const std::string& key);

  // Set the value at a given path
  void set(const std::string& key, const std::string& value);

  // Get the hash of a node at the given path. Empty std::string if no value is
  // set or there is no node at the given path
  std::string get_node_hash(const std::string& path);

  // Get the children as chars //TODO(Samir:) change to to return sting path to
  // children with value.
  std::vector<std::string> get_node_children(const std::string& path);

  // Erase previously set element in the trie
  void erase(const std::string& path);

  // delete subtree with root the node at the given path
  void delete_node_tree(const std::string& path);

  // finalizes the changes made by set
  void commit_changes();

  // discards the changes made by set;
  void discard_changes();

  // get the size of the hash in bytes
  uint32_t hash_size();

  // compare differences in with the second state and print path
  void print_subtrie(std::string path, std::string formated_path);

  uint32_t size();

 private:
  struct node {
    uint32_t parent;
    std::string prefix;
    std::string hash;
    std::string value;
    uint32_t children[256];
  };
  std::vector<node> nodes;
  std::map<uint32_t, node> backup;
  std::set<uint32_t> free_locations;
  crypto::hash_transformation* hasher;
  uint32_t nodes_current_state;
  uint32_t permanent_nodes_count;

  int32_t get_node_index(const std::string& path);
  bool has_children(uint32_t node_index);
  uint32_t add_node(uint32_t from, uint8_t to);
  // This needs to be called at the end of set() and erase() to recalculate the
  // hashes of all nodes from lowest child that was changed to the root
  void calculate_hash(uint32_t cur_node);
  // Create a backup starting from cur_node to root if they are not
  // in the backup map
  void backup_nodes(uint32_t cur_node);
  // add all nodes in the subtrie to free_locations to be reused.
  void subtrie_mark_free(uint32_t cur_node);
};

}  // namespace state
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_STATE_STATE_IMPL_H_
