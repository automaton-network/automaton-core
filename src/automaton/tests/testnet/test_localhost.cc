#include "automaton/core/data/factory.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/node/node.h"
#include "automaton/core/node/lua_node/lua_node.h"
#include "automaton/core/smartproto/smart_protocol.h"
#include "automaton/core/testnet/testnet.h"

#include "gtest/gtest.h"

using automaton::core::node::node;
using automaton::core::node::luanode::lua_node;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

TEST(testnet, test_all) {
  node::register_node_type("lua", [](const std::string& id, const std::string& proto_id)->std::shared_ptr<node> {
      return std::shared_ptr<node>(new lua_node(id, proto_id));
    });

  EXPECT_EQ(smart_protocol::load("chat", "automaton/tests/testnet/testproto/"), true);

  automaton::core::network::tcp_init();

  EXPECT_EQ(testnet::create_testnet("lua", "testnet", "chat", testnet::network_protocol_type::localhost, 5,
      {
        {1, {2, 3}}, {2, {4, 5}}
      }),
      true);

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
  EXPECT_EQ(node::list_nodes().size(), 0);

  automaton::core::network::tcp_release();
}
