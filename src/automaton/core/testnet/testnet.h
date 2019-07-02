#ifndef AUTOMATON_CORE_TESTNET_TESTNET_H_
#define AUTOMATON_CORE_TESTNET_TESTNET_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace automaton {
namespace core {
namespace testnet {

class testnet {
 public:
  enum network_protocol_type {
    localhost = 1,
    simulation = 2
  };

  ~testnet();

  static bool create_testnet(const std::string& node_type, const std::string& id, const std::string& smart_protocol_id,
      network_protocol_type ntype, uint32_t number_nodes, std::unordered_map<uint32_t,
      std::vector<uint32_t> > peer_list);

  static void destroy_testnet(const std::string& id);

  static std::shared_ptr<testnet> get_testnet(const std::string& id);

  static std::vector<std::string> list_testnets();

  void connect(const std::unordered_map<uint32_t, std::vector<uint32_t> >& peers_list) const;

  std::vector<std::string> list_nodes();

 private:
  static std::unordered_map<std::string, std::shared_ptr<testnet>> testnets;

  std::string node_type;
  std::string network_id;
  std::string protocol_id;
  network_protocol_type network_type;
  uint32_t number_nodes;
  std::vector<std::string> node_ids_list;  // <network_id>_1, <network_id>_2, ...

  testnet(const std::string& node_type, const std::string& id, const std::string& smart_protocol_id,
      network_protocol_type ntype, uint32_t number_nodes);

  bool init();
};

// Helper functions

std::unordered_map<uint32_t, std::vector<uint32_t> > create_connections_vector(uint32_t n, uint32_t p);
std::unordered_map<uint32_t, std::vector<uint32_t> > create_rnd_connections_vector(uint32_t n, uint32_t p);

}  // namespace testnet
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_TESTNET_TESTNET_H_
