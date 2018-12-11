#ifndef AUTOMATON_CORE_STATE_DUMMY_STATE_H__
#define AUTOMATON_CORE_STATE_DUMMY_STATE_H__

#include <map>
#include <string>
#include <vector>
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/state/state.h"

namespace automaton {
namespace core {
namespace state {

// Dummy state implementation, to be used for reference and tests.
// Peculiar traits worth noting:
// - get_node_hash is highly inefficient as it recalculates it from scratch.
// - get_node_children is always empty as there aren't actual nodes.
// - std::map is used to store current state / pending_changes.
class dummy_state : public state {
 public:
  explicit dummy_state(hash_transformation * hash);

  virtual std::string get(std::string key);
  virtual void set(std::string key, std::string value);
  virtual std::string get_node_hash(std::string path);
  virtual std::vector<std::string> get_node_children(std::string path);
  virtual void delete_node_tree(std::string path);
  virtual void commit_changes();
  virtual void discard_changes();

 private:
  hash_transformation * hash;
  std::map<std::string, std::string> data;
  std::map<std::string, std::string> pending_changes;
};

}  // namespace state
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_STATE_DUMMY_STATE_H__
