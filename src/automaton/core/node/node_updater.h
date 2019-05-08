#ifndef AUTOMATON_CORE_NODE_NODE_UPDATER_H_
#define AUTOMATON_CORE_NODE_NODE_UPDATER_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "automaton/core/node/node.h"

namespace automaton {
namespace core {
namespace node {

class node_updater {
 public:
  node_updater(uint32_t workers_number, uint32_t sleep_time, std::set<std::string> node_list);

  virtual void update_function() = 0;

  void add_node(const std::string& node_id);

  void remove_node(const std::string& node_id);

  void start();

  void stop();

 protected:
  bool running;
  std::set<std::string> node_list;

 private:
  std::mutex list_mutex;
  std::vector<std::thread> worker_threads;
  uint32_t workers_number;
  uint64_t sleep_time;
};

class default_node_updater : public node_updater {
 public:
  default_node_updater(uint32_t workers_number, uint32_t sleep_time, std::set<std::string> node_list);

  void update_function();

 private:
  std::unordered_map<std::string, std::unique_ptr<std::mutex> > node_locks;
  std::mutex map_lock;
};

class node_updater_tests : public node_updater {
 public:
  node_updater_tests(uint32_t sleep_time, std::set<std::string> node_list);

  void update_function();
};

}  // namespace node
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_NODE_NODE_UPDATER_H_
