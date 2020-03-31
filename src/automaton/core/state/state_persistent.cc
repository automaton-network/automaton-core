#include "automaton/core/state/state_persistent.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/io/io.h"

namespace automaton {
namespace core {
namespace state {

#define nodes (*p_nodes)
typedef std::basic_string<unsigned char> ustring;


state_persistent::state_persistent(crypto::hash_transformation* hasher,
                                   storage::blobstore* bs,
                                   storage::persistent_vector<node>* p_nodes
                                  )
  :bs(bs),
  p_nodes(p_nodes) {
  bs->store(0, nullptr);
  nodes.push_back(state_persistent::node());
  this->hasher = hasher;
  nodes_current_state = 1;
  calculate_hash(0);
  permanent_nodes_count = 1;
}

std::string state_persistent::get(const std::string& key) {
  int32_t node_index = get_node_index(key);
  //  std::cout << "index at key\"" << key << "\": " << node_index << std::endl;
  return node_index == -1 ? "" : nodes[node_index].get_value(bs);
}

void state_persistent::set(const std::string& key, const std::string& value) {
  if (value == "") {
    return;
    erase(key);
  }
  uint32_t cur_node = 0;
  uint32_t cur_prefix_index = 0;
  unsigned int i = 0;
  for (; i < key.length(); ++i) {
    unsigned char path_element = key[i];
    if (cur_prefix_index == nodes[cur_node].get_prefix(bs).length()) {
      // If there is no prefix or there is prefix but we have reached the end
      if (nodes[cur_node].get_child(path_element, bs) != 0) {
        // If there is child with the next path element continue on the path
        cur_node = nodes[cur_node].get_child(path_element, bs);
      } else if (has_children(cur_node) ||
          nodes[cur_node].get_value(bs) != "" || cur_node == 0) {
        // This node has children, value set or is the root so
        // It can't be the final node.
        backup_nodes(cur_node);
        cur_node = add_node(cur_node, path_element);
      } else {
        // this node has no children so it's the final node. The remainder
        // of the path will be the prefix including the path from parent
        nodes[cur_node].set_prefix(nodes[cur_node].get_prefix(bs) + key.substr(i), bs);
        break;
      }
      cur_prefix_index = 1;
    // If there is prefix at this node -> progress prefix
    } else {
      // If next path element does not match the next prefix element,
      // create a split node at the difference
      if (path_element !=
          (unsigned char)nodes[cur_node].get_prefix(bs)[cur_prefix_index]) {
        const std::string cur_node_prefix = nodes[cur_node].get_prefix(bs);

        // Backup necesery nodes before changes
        backup_nodes(cur_node);
        // Create the split_node and set up links with cur_node
        uint32_t split_node = add_node(nodes[cur_node].get_parent(bs),
            nodes[cur_node].get_prefix(bs)[0]);
        // Set split node as parent of cur_node
        nodes[cur_node].set_parent(split_node, bs);
        // Set the current node as child of the new split node
        unsigned char path_to_child =
            (unsigned char)cur_node_prefix[cur_prefix_index];
        nodes[split_node].set_child(path_to_child, cur_node, bs);

        // set prefix of split_node and cur_node
        nodes[split_node].set_prefix(cur_node_prefix.substr(0, cur_prefix_index), bs);
        nodes[cur_node].set_prefix(cur_node_prefix.substr(cur_prefix_index), bs);

        // Recalculate the hash of the child part of the node that got split
        calculate_hash(cur_node);
        // Create the new node from the split to the next path element
        cur_node = add_node(split_node, path_element);
        cur_prefix_index = 1;
      } else {
        ++cur_prefix_index;
      }
    }
  }

  // We checked the whole key but there is still prefix left.
  if ((nodes[cur_node].get_prefix(bs).length() != cur_prefix_index)
      && i == key.length()) {
    backup_nodes(cur_node);
    const std::string cur_node_prefix = nodes[cur_node].get_prefix(bs);

    // Create the split_node and set up links with cur_node
    uint32_t split_node = add_node(nodes[cur_node].get_parent(bs),
        nodes[cur_node].get_prefix(bs)[0]);
    // Set split node as parent of cur_node
    nodes[cur_node].set_parent(split_node, bs);

    // Set the current node as child of the new split node
    const unsigned char path_to_child = cur_node_prefix[cur_prefix_index];
    nodes[split_node].set_child(path_to_child, cur_node, bs);

    // set prefix of split_node and cur_node
    nodes[split_node].set_prefix(cur_node_prefix.substr(0, cur_prefix_index), bs);
    nodes[cur_node].set_prefix(cur_node_prefix.substr(cur_prefix_index), bs);

    // Recalculate the hash of the child part of the node that got split
    calculate_hash(cur_node);
    // Create the new node from the split to the next path element
    cur_node = split_node;
  }

  backup_nodes(cur_node);
  // set the value and calculate the hash
  nodes[cur_node].set_value(value, bs);
  calculate_hash(cur_node);
}

std::string state_persistent::get_node_hash(const std::string& path) {
  int32_t node_index = get_node_index(path);
  return node_index == -1 ? "" : nodes[node_index].get_hash(bs);
}

  std::vector<std::string> state_persistent::get_node_children(
      const std::string& path) {
  std::vector<std::string> result;
  int32_t node_index = get_node_index(path);
  if (node_index == -1) {
    throw std::out_of_range("No node at this path");
  }
  unsigned char i = 0;
  do {
    if (nodes[node_index].get_child(i, bs)) {
      uint32_t child = nodes[node_index].get_child(i, bs);
      // TODO(Samir): potential bug ( big vs little endian)
      result.push_back(std::string(nodes[child].get_prefix(bs)));
    }
  } while (++i != 0);
  return result;
}

void state_persistent::delete_node_tree(const std::string& path) {
  // TODO(Samir): Implement delete subtrie ( subtrie of node with value only? )
  int32_t cur_node = get_node_index(path);
  if (cur_node == -1 || nodes[cur_node].get_value(bs) == "") {
    throw std::out_of_range("In delete_node_tree: No set node at path: " + io::bin2hex(path));
  }
  backup_nodes(nodes[cur_node].get_parent(bs));
  subtrie_mark_free(cur_node);

  std::vector<unsigned char> children;
  unsigned char i = 0;
  uint32_t parent = nodes[cur_node].get_parent(bs);
  unsigned char path_from_parent = nodes[cur_node].get_prefix(bs)[0];
  nodes[parent].set_child(path_from_parent, 0, bs);
  free_locations.insert(cur_node);
  // move_last_element_to(cur_node);
  // Find out how many children does the parent have
  children.clear();
  cur_node = parent;
  // TODO(Samir): add this and all child nodes to fragmented locations
  do {
    if (nodes[cur_node].get_child(i, bs)) {
      children.push_back(i);
    }
  } while (++i != 0);
  // If the parent of the deleted node has no prefix, has only one
  // child remaining and is not the root we will merge it with his child
  if (nodes[cur_node].get_value(bs).length() == 0
      && children.size() == 1 && cur_node != 0) {
    parent = nodes[cur_node].get_parent(bs);
    uint32_t child = nodes[cur_node].get_child(children[0], bs);
    backup_nodes(child);

    // set prefix
    std::string new_perfix = nodes[child].get_prefix(bs);
    new_perfix.insert(0, nodes[cur_node].get_prefix(bs));
    nodes[child].set_prefix(new_perfix, bs);

    // link parent and child
    path_from_parent = nodes[cur_node].get_prefix(bs)[0];
    nodes[parent].set_child(path_from_parent, child, bs);
    nodes[child].set_parent(parent, bs);
    free_locations.insert(cur_node);
    // add_fragmented_location(cur_node);
    // move_last_element_to(cur_node);
    cur_node = child;
  }
  calculate_hash(cur_node);
}

// 1. If multiple children -> set value to ""
// 2. If one child -> merge with child and parent points to child
// 3. If no children, -> delete and remove link from parent,
//  3.1 If parent has only one child remaining, and the value of parent is "",
//      merge parent and its remaining child
void state_persistent::erase(const std::string& path) {
  int32_t cur_node = get_node_index(path);
  if (cur_node == -1 || nodes[cur_node].get_value(bs) == "") {
    throw std::out_of_range("In erase: No set node at path: " + io::bin2hex(path));
  }

  backup_nodes(cur_node);
  // Get the children of this node
  std::vector<unsigned char> children;
  unsigned char i = 0;
  do {
    if (nodes[cur_node].get_child(i, bs)) {
      children.push_back(i);
    }
  } while (++i != 0);

  // If multiple children just erase the value
  if (children.size() > 1) {
    nodes[cur_node].set_value("", bs);
  // If one child -> merge prefix into child, link parent and child
  } else if (children.size() == 1) {
    uint32_t parent = nodes[cur_node].get_parent(bs);
    uint32_t child = nodes[cur_node].get_child(children[0], bs);
    // backup the child before chaning it
    backup_nodes(child);
    // add the prefix of current node to the child
    std::string new_perfix = nodes[child].get_prefix(bs);
    new_perfix.insert(0, nodes[cur_node].get_prefix(bs));
    nodes[child].set_prefix(new_perfix, bs);
    // link parent and child
    unsigned char path_from_parent = nodes[cur_node].get_prefix(bs)[0];
    nodes[parent].set_child(path_from_parent, child, bs);
    nodes[child].set_parent(parent, bs);
    // Remember empty elements for later use
    free_locations.insert(cur_node);
    // move_last_element_to(cur_node);
    cur_node = child;
  // If no child -> remove element and handle parent cases
  } else {
    // erase the link from parent
    uint32_t parent = nodes[cur_node].get_parent(bs);
    unsigned char path_from_parent = nodes[cur_node].get_prefix(bs)[0];
    nodes[parent].set_child(path_from_parent, 0, bs);
    free_locations.insert(cur_node);
    // move_last_element_to(cur_node);
    // Find out how many children does the parent have
    children.clear();
    cur_node = parent;
    unsigned char z = 0;
    do {
      if (nodes[cur_node].get_child(z, bs)) {
        children.push_back(z);
      }
    } while (++z != 0);
    // If the parent of the deleted node has no prefix, has only one
    // child remaining and is not the root we will merge it with his child
    if (nodes[cur_node].get_value(bs).length() == 0
        && children.size() == 1 && cur_node != 0) {
      parent = nodes[cur_node].get_parent(bs);
      uint32_t child = nodes[cur_node].get_child(children[0], bs);
      backup_nodes(child);
      std::string new_perfix = nodes[child].get_prefix(bs);
      new_perfix.insert(0, nodes[cur_node].get_prefix(bs));
      nodes[child].set_prefix(new_perfix, bs);

      // link parent and child
      path_from_parent = nodes[cur_node].get_prefix(bs)[0];
      nodes[parent].set_child(path_from_parent, child, bs);
      nodes[child].set_parent(parent, bs);
      free_locations.insert(cur_node);
      // move_last_element_to(cur_node);
      cur_node = child;
    }
  }
  calculate_hash(cur_node);
}

void state_persistent::commit_changes() {
  // Erase backups
  backup.clear();
  if (free_locations.empty()) {
    permanent_nodes_count = nodes.size();
    return;
  }
  // If we have fragmented, move not deleted elements from
  // the end of the vector into them
  auto it_low = free_locations.begin();
  auto rit_high = free_locations.rbegin();
  uint32_t empty_elements = free_locations.size();
  uint32_t last_element = nodes.size() - 1;
  // Copy elements and skip if element is deleted
  while (*it_low != *rit_high) {
  // auto it_last_element = free_locations.find(last_element);
    if (last_element == *rit_high) {
      rit_high++;
    } else {
      nodes[*it_low] = nodes[last_element];
      uint32_t parent = nodes[last_element].get_parent(bs);
      uint8_t path_from_parent = nodes[last_element].get_prefix(bs)[0];
      nodes[parent].set_child(path_from_parent, *it_low, bs);
      it_low++;
    }
    last_element--;
  }
  nodes[*it_low] = nodes[last_element];

  nodes.resize(nodes.size() - empty_elements);
  permanent_nodes_count = nodes.size();
  free_locations.clear();
}

void state_persistent::discard_changes() {
  for (auto it = backup.begin(); it != backup.end(); ++it) {
    nodes[it->first] = it->second;
  }
  nodes.resize(permanent_nodes_count);
  free_locations.clear();
}

uint32_t state_persistent::hash_size() {
  return hasher->digest_size();
}

void state_persistent::print_subtrie(std::string path, std::string formated_path) {
  std::cout << formated_path << " prefix: " <<
      io::bin2hex(nodes[get_node_index(path)].get_prefix(bs)) << " value: " << get(path)
      << " hash: " << io::bin2hex(get_node_hash(path)) << std::endl << std::endl;
  std::vector<std::string> children = get_node_children(path);
  for (auto i : children) {
    print_subtrie(path + i, formated_path + "/" + io::bin2hex(i));
  }
}

uint32_t state_persistent::size() {
  return nodes.size();
}

// TODO(Samir): Remove all calls to substring
int32_t state_persistent::get_node_index(const std::string& path) {
  uint32_t cur_node = 0;
  uint32_t i;
  bool key_ended_at_edge = false;
  for (i = 0; i < path.length(); i++) {
    key_ended_at_edge = false;
    uint8_t path_element = path[i];
    // if no prefix keep looking
    if ((int32_t)nodes[cur_node].get_prefix(bs).length()-1 <= 0) {
      if (nodes[cur_node].get_child(path_element, bs) == 0) {
        return -1;
      }
      cur_node = nodes[cur_node].get_child(path_element, bs);
      key_ended_at_edge = true;
    // else compare prefix with remaining path and decide what to do
    } else {
      // if prefix is shorter than remaining path, compare them.
      if ((int32_t)nodes[cur_node].get_prefix(bs).length()-1 <
          (int32_t)path.length() - (int32_t)i) {
        if (nodes[cur_node].get_prefix(bs) ==
            path.substr(i-1, nodes[cur_node].get_prefix(bs).length())) {
          i += (int32_t)nodes[cur_node].get_prefix(bs).length()-1;
          path_element = path[i];
          if (nodes[cur_node].get_child(path_element, bs)) {
            cur_node = nodes[cur_node].get_child(path_element, bs);
            key_ended_at_edge = true;
          } else {
            return -1;
          }
        } else {
          return -1;
        }
      // if prefix length is equal to remaining path compare
      } else if ((int32_t)nodes[cur_node].get_prefix(bs).length()-1
            == (int32_t)path.length() - (int32_t)i) {
        if (nodes[cur_node].get_prefix(bs) == path.substr(i-1)) {
          return cur_node;
        } else {
          return -1;
        }
      // if prefix is longer than remaining path there is no node with this path
      } else {
        return -1;
      }
    }
  }
  if (key_ended_at_edge && (int32_t)nodes[cur_node].get_prefix(bs).length()-1) {
    return -1;
  }
  return cur_node;
}


bool state_persistent::has_children(uint32_t node_index) {
  for (unsigned int i = 0; i < 256; ++i) {
    if (nodes[node_index].get_child(static_cast<uint8_t>(i), bs)) {
      return true;
    }
  }
  return false;
}

uint32_t state_persistent::add_node(uint32_t from, unsigned char to) {
  uint32_t new_node;
  // Add node to the end of the vector if the are no fragmented location
  if (free_locations.empty()) {
    new_node = nodes.size();
    nodes.push_back(node());
  } else {
    auto it_fragmented_locations =  free_locations.begin();
    backup_nodes(*it_fragmented_locations);
    new_node = *it_fragmented_locations;
    // TODO(Samir): change to emplace(node)
    nodes[new_node] = node();
    free_locations.erase(it_fragmented_locations);
  }
  nodes[from].set_child(to, new_node, bs);
  nodes[new_node].set_parent(from, bs);
  nodes[new_node].set_prefix(std::string(reinterpret_cast<char*>(&to), 1), bs);
  return new_node;
}

void state_persistent::calculate_hash(uint32_t cur_node) {
  const uint8_t *value;
  const uint8_t *prefix;
  const uint8_t *child_hash;

  uint32_t len = 0;
  hasher->restart();  // just in case
  // If we are at root and we have no children the hash will be ""
  if (!cur_node && !has_children(cur_node)) {
    nodes[cur_node].set_hash("", bs);
    return;
  }

  // Hash the value
  std::string str_value = nodes[cur_node].get_value(bs);
  value =
      reinterpret_cast<const uint8_t*>(str_value.data());
  len = nodes[cur_node].get_value(bs).length();
  hasher->update(value, len);

  // Hash the prefix
  std::string str_prefix = nodes[cur_node].get_prefix(bs);
  prefix =
      reinterpret_cast<const uint8_t*>(str_prefix.data());
  len = nodes[cur_node].get_prefix(bs).length();
  hasher->update(prefix, len);
  // Hash the children hashes
  for (int i = 0; i < 256; i++) {
    if (nodes[cur_node].get_child(i, bs)) {
      uint32_t child = nodes[cur_node].get_child(i, bs);

      std::string str_child_hash = nodes[child].get_hash(bs);
      child_hash = reinterpret_cast<const uint8_t*>(str_child_hash.data());
      len = nodes[child].get_hash(bs).length();
      hasher->update(child_hash, len);
    }
  }
  uint8_t * digest = new uint8_t[hasher->digest_size()];
  hasher->final(digest);
  nodes[cur_node].set_hash(
      std::string(reinterpret_cast<char*>(digest), hasher->digest_size()), bs);
  delete[] digest;
  if (cur_node != 0) {
    calculate_hash(nodes[cur_node].get_parent(bs));
  }
}

void state_persistent::backup_nodes(uint32_t cur_node) {
  // Insert element if it is not in the map. If sucsessfully inserted keep
  // calling backup_nodes on the parent unless we have reached the root
  if (backup.insert(std::make_pair((int32_t)cur_node,
      nodes[cur_node])).second && cur_node
      && cur_node < permanent_nodes_count) {
    backup_nodes(nodes[cur_node].get_parent(bs));
  }
}

void state_persistent::subtrie_mark_free(uint32_t cur_node) {
  uint8_t child = 0;
  free_locations.insert(cur_node);
  do {
    if (nodes[cur_node].get_child(child, bs)) {
      subtrie_mark_free(nodes[cur_node].get_child(child, bs));
    }
  } while (++child != 0);
  return;
}


uint32_t state_persistent::node::get_parent(storage::blobstore * _bs) {
  uint32_t sz;
  uint8_t* p_parent = _bs->get(parent_, &sz);
  return *(reinterpret_cast<uint32_t*>(p_parent));
}

std::string state_persistent::node::get_prefix(storage::blobstore * _bs) {
  uint32_t sz;
  uint8_t* p_prefix = _bs->get(prefix_, &sz);
  return std::string(reinterpret_cast<char*>(p_prefix), sz);
}

std::string state_persistent::node::get_hash(storage::blobstore * _bs) {
  uint32_t sz;
  uint8_t* p_hash = _bs->get(hash_, &sz);
  return std::string(reinterpret_cast<char*>(p_hash), sz);
  // return hash_;
}

std::string state_persistent::node::get_value(storage::blobstore * _bs) {
  uint32_t sz;
  uint8_t* p_value = _bs->get(value_, &sz);
  return std::string(reinterpret_cast<char*>(p_value), sz);
}

uint32_t state_persistent::node::get_child(uint8_t child, storage::blobstore * _bs) {
  return children_[child];
}

void state_persistent::node::set_parent(uint32_t parent, storage::blobstore * _bs) {
  parent_ = _bs->store(sizeof(uint32_t), reinterpret_cast<uint8_t*>(&parent));
}

void state_persistent::node::set_prefix(const std::string prefix, storage::blobstore * _bs) {
  prefix_ = _bs->store(prefix.length(), reinterpret_cast<const uint8_t*>(prefix.data()));
}

void state_persistent::node::set_hash(const std::string hash, storage::blobstore * _bs) {
  hash_ = _bs->store(hash.length(), reinterpret_cast<const uint8_t*>(hash.data()));
}

void state_persistent::node::set_value(const std::string value, storage::blobstore * _bs) {
  value_ = _bs->store(value.length(),
    reinterpret_cast<const uint8_t*>(value.data()));
}

void state_persistent::node::set_child(const uint8_t child,
                                       const uint32_t value,
                                       storage::blobstore * _bs) {
  children_[child] = value;
}

}  // namespace state
}  // namespace core
}  // namespace automaton
