#include "automaton/core/data/factory.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/node/node.h"
#include "automaton/core/smartproto/smart_protocol.h"
#include "automaton/core/testnet/testnet.h"

#include "gtest/gtest.h"

using automaton::core::node::node;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

TEST(testnet, test_all) {
  EXPECT_EQ(smart_protocol::load("chat", "automaton/tests/testnet/testproto/"), true);

  std::shared_ptr<automaton::core::network::simulation> sim = automaton::core::network::simulation::get_simulator();
  sim->simulation_start(50);

  testnet::create_testnet("testnet", "chat", testnet::network_protocol_type::simulation, 5,
      {
        {1, {2, 3}}, {2, {4, 5}}
      });

  auto net = testnet::get_testnet("testnet");
  auto msg_factory = smart_protocol::get_protocol("chat")->get_factory();
  auto msg1 = msg_factory->new_message_by_name("Peers");
  auto msg2 = msg_factory->new_message_by_name("Peers");

  // There should be only one testnet named "testnet"
  EXPECT_EQ(testnet::list_testnets().size(), 1);
  EXPECT_EQ(testnet::list_testnets()[0], "testnet");

  // Wait until all nodes receive the message
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));

  // Node "testnet_4" should have only one peer - "testnet_2"
  std::string result1 = node::get_node("testnet_4")->process_cmd("get_peers", "");
  msg1->deserialize_message(result1);
  EXPECT_EQ(msg1->get_repeated_field_size(1), 1);
  EXPECT_EQ(msg1->get_repeated_blob(1, 0), "testnet_2");

  std::string result2 = node::get_node("testnet_3")->process_cmd("get_heard_of", "");
  msg2->deserialize_message(result2);
  // Nodes should have heard of all 5 peers
  EXPECT_EQ(msg2->get_repeated_field_size(1), 5);

  // Sort nodes names
  std::set<std::string> result;
  for (uint32_t i = 0; i < 5; ++i) {
    result.insert(msg2->get_repeated_blob(1, i));
  }
  auto it = result.begin();
  for (uint32_t i = 1; it != result.end(), i <= 5; ++it, ++i) {
    EXPECT_EQ(*it, "testnet_" + std::to_string(i));
  }

  testnet::destroy_testnet("testnet");
  EXPECT_EQ(testnet::list_testnets().size(), 0);

  sim->simulation_stop();
}
