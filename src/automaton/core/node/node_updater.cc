#include "automaton/core/node/node_updater.h"

namespace automaton {
namespace core {
namespace node {

node_updater::node_updater(uint32_t workers_number, uint32_t sleep_time, std::set<std::string> node_list):
    node_list(node_list), workers_number(workers_number), sleep_time(sleep_time) {}

void node_updater::add_node(const std::string& node_id) {
  std::lock_guard<std::mutex> lock(list_mutex);
  node_list.insert(node_id);
}

void node_updater::remove_node(const std::string& node_id) {
  std::lock_guard<std::mutex> lock(list_mutex);
  node_list.erase(node_id);
}

void node_updater::start() {
  running = true;
  for (uint32_t i = 0; i < workers_number; ++i) {
    worker_threads.emplace_back(std::thread([&]() {
      while (running) {
        update_function();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
      }
    }));
  }
}

void node_updater::stop() {
  if (!running) {
    return;
  }
  running = false;
  for (uint32_t i = 0; i < workers_number; ++i) {
    worker_threads[i].join();
  }
}

default_node_updater::default_node_updater(uint32_t workers_number, uint32_t sleep_time,
    std::set<std::string> node_list):node_updater(workers_number, sleep_time, node_list) {}

void default_node_updater::update_function() {
  for (auto nit = node_list.begin(); nit != node_list.end(); ++nit) {
    if (!running) {
      break;
    }
    std::string id = *nit;
    map_lock.lock();
    auto it = node_locks.find(id);
    if (it == node_locks.end()) {
      it = node_locks.emplace(id, std::make_unique<std::mutex>()).first;
    }
    bool locked = it->second->try_lock();
    map_lock.unlock();
    if (locked) {
      auto node = node::get_node(id);
      if (node != nullptr) {
        uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (current_time >= (node->get_time_to_update())) {
          node->process_update(current_time);
        }
      } else {
        remove_node(id);
      }
      map_lock.lock();
      node_locks.at(id)->unlock();
      map_lock.unlock();
    } else {
      continue;
    }
  }
}

node_updater_tests::node_updater_tests(uint32_t sleep_time, std::set<std::string> node_list):
    node_updater(1, sleep_time, node_list) {}

void node_updater_tests::update_function() {
  auto it = node_list.begin();
  std::advance(it, std::rand()%(node_list.size() - 1));
  auto node = node::get_node(*it);
  if (node != nullptr) {
    // protocol update time and node's time to update are not used here
    node->process_update(0);
  }
}
}  // namespace node
}  // namespace core
}  // namespace automaton
