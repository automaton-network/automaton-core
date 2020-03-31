#include "automaton/core/state/state_impl.h"

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

typedef std::basic_string<uint8_t> ustring;

state_impl::state_impl(crypto::hash_transformation* hasher) {
  nodes.push_back(state_impl::node());
  this->hasher = hasher;
  nodes_current_state = 1;
  calculate_hash(0);
  permanent_nodes_count = 1;
}

std::string state_impl::get(const std::string& key) {
  int32_t node_index = get_node_index(key);
  //  std::cout << "index at key\"" << key << "\": " << node_index << std::endl;
  return node_index == -1 ? "" : nodes[node_index].value;
}
void state_impl::set(const std::string& key, const std::string& value) {
  if (value == "") {
    erase(key);
    return;
  }
  uint32_t cur_node = 0;
  uint32_t cur_prefix_index = 0;
  uint32_t i = 0;
  for (; i < key.length(); ++i) {
    uint8_t path_element = key[i];
    if (cur_prefix_index == nodes[cur_node].prefix.length()) {
      // If there is no prefix or there is prefix but we have reached the end
      if (nodes[cur_node].children[path_element] != 0) {
        // If there is child with the next path element continue on the path
        cur_node = nodes[cur_node].children[path_element];
      } else if (has_children(cur_node) ||
          nodes[cur_node].value != "" || cur_node == 0) {
        // This node has children, value set or is the root.
        // It can't be the final node.
        backup_nodes(cur_node);
        cur_node = add_node(cur_node, path_element);
      } else {
        // this node has no children so it's the final node. The remainder
        // of the path will be the prefix including the path from parent
        nodes[cur_node].prefix.append(key.substr(i));
        break;
      }
      cur_prefix_index = 1;
    // If there is prefix at this node -> progress prefix
    } else {
      // If next path element does not match the next prefix element,
      // create a split node at the difference
      if (path_element !=
          (uint8_t)nodes[cur_node].prefix[cur_prefix_index]) {
        const std::string cur_node_prefix = nodes[cur_node].prefix;

        // Backup necesery nodes before changes
        backup_nodes(cur_node);
        // Create the split_node and set up links with cur_node
        uint32_t split_node = add_node(nodes[cur_node].parent,
            nodes[cur_node].prefix[0]);
        // Set split node as parent of cur_node
        nodes[cur_node].parent = split_node;
        // Set the current node as child of the new split node
        uint8_t path_to_child =
            (uint8_t)cur_node_prefix[cur_prefix_index];
        nodes[split_node].children[path_to_child] = cur_node;

        // set prefix of split_node and cur_node
        nodes[split_node].prefix = cur_node_prefix.substr(0, cur_prefix_index);
        nodes[cur_node].prefix = cur_node_prefix.substr(cur_prefix_index);

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

  // We checked the whoel key but there is still prefix left.
  if ((nodes[cur_node].prefix.length() != cur_prefix_index)
      && i == key.length()) {
    backup_nodes(cur_node);
    const std::string cur_node_prefix = nodes[cur_node].prefix;

    // Create the split_node and set up links with cur_node
    uint32_t split_node = add_node(nodes[cur_node].parent,
        nodes[cur_node].prefix[0]);
    // Set split node as parent of cur_node
    nodes[cur_node].parent = split_node;

    // Set the current node as child of the new split node
    const uint8_t path_to_child = cur_node_prefix[cur_prefix_index];
    nodes[split_node].children[path_to_child] = cur_node;

    // set prefix of split_node and cur_node
    nodes[split_node].prefix = cur_node_prefix.substr(0, cur_prefix_index);
    nodes[cur_node].prefix = cur_node_prefix.substr(cur_prefix_index);

    // Recalculate the hash of the child part of the node that got split
    calculate_hash(cur_node);
    // Create the new node from the split to the next path element
    cur_node = split_node;
  }

  backup_nodes(cur_node);
  // set the value and calculate the hash
  nodes[cur_node].value = value;
  calculate_hash(cur_node);
}

std::string state_impl::get_node_hash(const std::string& path) {
  int32_t node_index = get_node_index(path);
  return node_index == -1 ? "" : nodes[node_index].hash;
}

  std::vector<std::string> state_impl::get_node_children(
      const std::string& path) {
  std::vector<std::string> result;
  int32_t node_index = get_node_index(path);
  if (node_index == -1) {
    std::stringstream msg;
    msg << "No node at this path";
    //  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }
  uint8_t i = 0;
  do {
    if (nodes[node_index].children[i]) {
      uint32_t child = nodes[node_index].children[i];
      // TODO(Samir): potential bug ( big vs little endian)
      result.push_back(std::string(nodes[child].prefix));
    }
  } while (++i != 0);
  return result;
}

void state_impl::delete_node_tree(const std::string& path) {
  // TODO(Samir): Implement delete subtrie ( subtrie of node with value only? )
  int32_t cur_node = get_node_index(path);
  if (cur_node == -1 || nodes[cur_node].value == "") {
    std::stringstream msg;
    msg << "No set node at path: " << io::bin2hex(path);
    //  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }
  backup_nodes(nodes[cur_node].parent);
  subtrie_mark_free(cur_node);

  std::vector<uint8_t> children;
  uint8_t i = 0;
  uint32_t parent = nodes[cur_node].parent;
  uint8_t path_from_parent = nodes[cur_node].prefix[0];
  nodes[parent].children[path_from_parent] = 0;
  free_locations.insert(cur_node);
  // move_last_element_to(cur_node);
  // Find out how many children does the parent have
  children.clear();
  cur_node = parent;
  // TODO(Samir): add this and all child nodes to fragmented locations
  do {
    if (nodes[cur_node].children[i]) {
      children.push_back(i);
    }
  } while (++i != 0);
  // If the parent of the deleted node has no prefix, has only one
  // child remaining and is not the root we will merge it with his child
  if (nodes[cur_node].value.length() == 0
    && children.size() == 1 && cur_node != 0) {
    parent = nodes[cur_node].parent;
    uint32_t child = nodes[cur_node].children[children[0]];
    backup_nodes(child);
    nodes[child].prefix.insert(0, nodes[cur_node].prefix);
    // link parent and child
    path_from_parent = nodes[cur_node].prefix[0];
    nodes[parent].children[path_from_parent] = child;
    nodes[child].parent = parent;
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
void state_impl::erase(const std::string& path) {
  int32_t cur_node = get_node_index(path);
  if (cur_node == -1 || nodes[cur_node].value == "") {
    std::stringstream msg;
    msg << "No set node at path: " << io::bin2hex(path);
    //  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }

  backup_nodes(cur_node);
  // Get the children of this node
  std::vector<uint8_t> children;
  uint8_t i = 0;
  do {
    if (nodes[cur_node].children[i]) {
      children.push_back(i);
    }
  } while (++i != 0);

  // If multiple children just erase the value
  if (children.size() > 1) {
    nodes[cur_node].value = "";
  // If one child -> merge prefix into child, link parent and child
  } else if (children.size() == 1) {
    uint32_t parent = nodes[cur_node].parent;
    uint32_t child = nodes[cur_node].children[children[0]];
    // backup the child before chaning it
    backup_nodes(child);
    // add the prefix of current node to the child
    nodes[child].prefix.insert(0, nodes[cur_node].prefix);
    // link parent and child
    uint8_t path_from_parent = nodes[cur_node].prefix[0];
    nodes[parent].children[path_from_parent] = child;
    nodes[child].parent = parent;
    // Remember empty elements for later use
    free_locations.insert(cur_node);
    // move_last_element_to(cur_node);
    cur_node = child;
  // If no child -> remove element and handle parent cases
  } else {
    // erase the link from parent
    uint32_t parent = nodes[cur_node].parent;
    uint8_t path_from_parent = nodes[cur_node].prefix[0];
    nodes[parent].children[path_from_parent] = 0;
    free_locations.insert(cur_node);
    // move_last_element_to(cur_node);
    // Find out how many children does the parent have
    children.clear();
    cur_node = parent;
    uint8_t z = 0;
    do {
      if (nodes[cur_node].children[z]) {
        children.push_back(z);
      }
    } while (++z != 0);
    // If the parent of the deleted node has no prefix, has only one
    // child remaining and is not the root we will merge it with his child
    if (nodes[cur_node].value.length() == 0
        && children.size() == 1 && cur_node != 0) {
      parent = nodes[cur_node].parent;
      uint32_t child = nodes[cur_node].children[children[0]];
      backup_nodes(child);
      nodes[child].prefix.insert(0, nodes[cur_node].prefix);

      // link parent and child
      path_from_parent = nodes[cur_node].prefix[0];
      nodes[parent].children[path_from_parent] = child;
      nodes[child].parent = parent;
      free_locations.insert(cur_node);
      // move_last_element_to(cur_node);
      cur_node = child;
    }
  }
  calculate_hash(cur_node);
}

void state_impl::commit_changes() {
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
      uint32_t parent = nodes[last_element].parent;
      uint8_t path_from_parent = nodes[last_element].prefix[0];
      nodes[parent].children[path_from_parent] = *it_low;
      it_low++;
    }
    last_element--;
  }
  nodes[*it_low] = nodes[last_element];

  nodes.resize(nodes.size() - empty_elements);
  permanent_nodes_count = nodes.size();
  free_locations.clear();
}

void state_impl::discard_changes() {
  for (auto it = backup.begin(); it != backup.end(); ++it) {
    nodes[it->first] = it->second;
  }
  nodes.resize(permanent_nodes_count);
  free_locations.clear();
}

uint32_t state_impl::hash_size() {
  return hasher->digest_size();
}

void state_impl::print_subtrie(std::string path, std::string formated_path) {
  std::cout << formated_path << " prefix: " <<
      io::bin2hex(nodes[get_node_index(path)].prefix) << " value: " << get(path)
      << " hash: " << io::bin2hex(get_node_hash(path)) << std::endl << std::endl;
  std::vector<std::string> children = get_node_children(path);
  for (auto i : children) {
    print_subtrie(path + i, formated_path + "/" + io::bin2hex(i));
  }
}
uint32_t state_impl::size() {
  return nodes.size();
}
// TODO(Samir): Remove all calls to substring
int32_t state_impl::get_node_index(const std::string& path) {
  uint32_t cur_node = 0;
  uint32_t i;
  bool key_ended_at_edge = false;
  for (i = 0; i < path.length(); i++) {
    key_ended_at_edge = false;
    uint8_t path_element = path[i];
    // if no prefix keep looking
    if ((int32_t)nodes[cur_node].prefix.length()-1 <= 0) {
      if (nodes[cur_node].children[path_element] == 0) {
        return -1;
      }
      cur_node = nodes[cur_node].children[path_element];
      key_ended_at_edge = true;
    // else compare prefix with remaining path and decide what to do
    } else {
      // if prefix is shorter than remaining path, compare them.
      if ((int32_t)nodes[cur_node].prefix.length()-1 <
          (int32_t)path.length() - (int32_t)i) {
        if (nodes[cur_node].prefix == path.substr(i-1,
            nodes[cur_node].prefix.length())) {
          i += (int32_t)nodes[cur_node].prefix.length()-1;
          path_element = path[i];
          if (nodes[cur_node].children[path_element]) {
            cur_node = nodes[cur_node].children[path_element];
            key_ended_at_edge = true;
          } else {
            return -1;
          }
        } else {
          return -1;
        }
      // if prefix length is equal to remaining path compare
      } else if ((int32_t)nodes[cur_node].prefix.length()-1
            == (int32_t)path.length() - (int32_t)i) {
        if (nodes[cur_node].prefix == path.substr(i-1)) {
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
  if (key_ended_at_edge && (int32_t)nodes[cur_node].prefix.length()-1) {
    return -1;
  }
  return cur_node;
}


bool state_impl::has_children(uint32_t node_index) {
  for (uint32_t i = 0; i < 256; ++i) {
    if (nodes[node_index].children[i]) {
      return true;
    }
  }
  return false;
}

uint32_t state_impl::add_node(uint32_t from, uint8_t to) {
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
  nodes[from].children[to] = new_node;
  nodes[new_node].parent = from;
  nodes[new_node].prefix = std::string(reinterpret_cast<char*>(&to), 1);
  return new_node;
}

void state_impl::calculate_hash(uint32_t cur_node) {
  const uint8_t *value, *prefix, *child_hash;
  hasher->restart();  // just in case
  // If we are at root and we have no children the hash will be ""
  if (!cur_node && !has_children(cur_node)) {
    nodes[cur_node].hash = "";
    return;
  }

  // Hash the value
  value =
      reinterpret_cast<const uint8_t*>(nodes[cur_node].value.data());
  uint32_t len = nodes[cur_node].value.length();
  hasher->update(value, len);

  // Hash the prefix
  prefix =
      reinterpret_cast<const uint8_t*>(nodes[cur_node].prefix.data());
  len = nodes[cur_node].prefix.length();
  hasher->update(prefix, len);

  // Hash the children hashes
  for (int i = 0; i < 256; i++) {
    if (nodes[cur_node].children[i]) {
      uint32_t child = nodes[cur_node].children[i];
      child_hash =
          reinterpret_cast<const uint8_t*>(nodes[child].hash.data());
      len = nodes[child].hash.length();
      hasher->update(child_hash, len);
    }
  }
  uint8_t * digest = new uint8_t[hasher->digest_size()];
  hasher->final(digest);
  nodes[cur_node].hash = std::string(reinterpret_cast<char*>(digest),
    hasher->digest_size());

  delete[] digest;
  if (cur_node != 0) {
    calculate_hash(nodes[cur_node].parent);
  }
}

void state_impl::backup_nodes(uint32_t cur_node) {
  // Insert element if it is not in the map. If sucsessfully inserted keep
  // calling backup_nodes on the parent unless we have reached the root
  if (backup.insert(std::make_pair((int32_t)cur_node,
      nodes[cur_node])).second && cur_node
    && cur_node < permanent_nodes_count) {
    backup_nodes(nodes[cur_node].parent);
  }
}

void state_impl::subtrie_mark_free(uint32_t cur_node) {
  uint8_t child = 0;
  free_locations.insert(cur_node);
  do {
    if (nodes[cur_node].children[child]) {
      subtrie_mark_free(nodes[cur_node].children[child]);
    }
  } while (++child != 0);
  return;
}

}  // namespace state
}  // namespace core
}  // namespace automaton
