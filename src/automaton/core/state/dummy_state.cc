#include "automaton/core/state/dummy_state.h"

#include <glog/logging.h>

#include <string>
#include <vector>
#include <utility>

dummy_state::dummy_state(hash_transformation * hash) : hash(hash) {
  google::InstallFailureSignalHandler();
}

std::string dummy_state::get(std::string key) {
  if (pending_changes.count(key)) {
    return pending_changes.at(key);
  }
  if (data.count(key)) {
    return data.at(key);
  }
  return "";
}

void dummy_state::set(std::string key, std::string value) {
  pending_changes.erase(key);
  pending_changes.insert(std::pair<std::string, std::string>(key, value));
}

std::string dummy_state::get_node_hash(std::string path) {
  LOG(INFO) << "SIZE: " << data.size() << std::endl;
  if (data.size() == 0) {
    return "";
  }

  hash->restart();
  for (auto kv : data) {
    hash->update((const uint8_t *)kv.first.data(), kv.first.size());
    hash->update((const uint8_t *)kv.second.data(), kv.second.size());
  }

  uint8_t * digest = new uint8_t[hash->digest_size()];
  hash->final(digest);
  std::string result((const char *)digest, hash->digest_size());
  delete[] digest;
  return result;
}

std::vector<std::string> dummy_state::get_node_children(std::string path) {
  return std::vector<std::string>();
}

void dummy_state::delete_node_tree(std::string path) {
  auto start = data.lower_bound(path);
  auto end = data.upper_bound(path + "\xFF");
  for (auto it = start; it != end; it++) {
    LOG(INFO) << "DELETING FROM MAIN STATE " << it->first << std::endl;
    pending_changes.erase(it->first);
    pending_changes.insert(std::pair<std::string, std::string>(it->first, ""));
  }

  start = pending_changes.lower_bound(path);
  end = pending_changes.upper_bound(path + "\xFF");
  for (auto it = start; it != end; it++) {
    if (it->second != "") {
      LOG(INFO) << "DELETING FROM PENDING CHANGES " << it->first << std::endl;
      pending_changes.erase(it->first);
      pending_changes.insert(
          std::pair<std::string, std::string>(it->first, ""));
    }
  }
}

void dummy_state::commit_changes() {
  LOG(INFO) << "COMMIT " << pending_changes.size() << " PENDING CHANGES"
      << std::endl;
  for (auto kv : pending_changes) {
    auto& key = kv.first;
    auto& value = kv.second;
    data.erase(key);
    if (value == "") {
      LOG(INFO) << "REMOVING " << key << std::endl;
    } else {
      LOG(INFO) << "ADDING " << key << ":" << value << std::endl;
      data.insert(kv);
    }
  }
  pending_changes.clear();
}

void dummy_state::discard_changes() {
  pending_changes.clear();
}
