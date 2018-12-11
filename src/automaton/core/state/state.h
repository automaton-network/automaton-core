#ifndef AUTOMATON_CORE_STATE_STATE_H_
#define AUTOMATON_CORE_STATE_STATE_H_

#include <string>
#include <vector>

namespace automaton {
namespace core {
namespace state {

class state {
 public:
  // Get the value at given path. Empty string if no value is set or
  // there is no node at the given path
  virtual std::string get(const std::string& key) = 0;

  // Set the value at a given path
  virtual void set(const std::string& key, const std::string& value) = 0;

  // Get the hash of a node at the given path. Empty std::string if no value is
  // set or there is no node at the given path
  virtual std::string get_node_hash(const std::string& path) = 0;

  // Get the children as chars
  virtual std::vector<std::string> get_node_children(
      const std::string& path) = 0;

  // Erase previously set element in the trie
  virtual void erase(const std::string& path) = 0;

  // Delete subtree with root the node at the given path
  virtual void delete_node_tree(const std::string& path) = 0;

  // Finalizes the changes made by set
  virtual void commit_changes() = 0;

  // Discards the changes made by set
  virtual void discard_changes() = 0;

  // Get the size of the hash in bytes
  virtual uint32_t hash_size() = 0;

  virtual ~state() {}
};

}  // namespace state
}  // namespace core
}  // namespace automaton

#endif  //  AUTOMATON_CORE_STATE_STATE_H_
