/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/data/factory.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/io/io.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/node/lua_node/lua_node.h"
#include "automaton/core/node/node.h"
#include "automaton/core/script/engine.h"
#include "automaton/core/smartproto/smart_protocol.h"
#include "automaton/core/state/state_impl.h"
#include "automaton/core/storage/persistent_blobstore.h"
#include "automaton/core/testnet/testnet.h"
#include <boost/filesystem.hpp>

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::io::bin2hex;
using automaton::core::node::luanode::lua_node;
using automaton::core::node::node;
using automaton::core::smartproto::smart_protocol;
using automaton::core::state::state_impl;
using automaton::core::storage::persistent_blobstore;
using automaton::core::testnet::testnet;

void test_state_impl_set_and_get() {
  std::vector<std::pair<std::string, std::string> > tests;
  tests.push_back(std::make_pair("test", "1"));
  tests.push_back(std::make_pair("tester", "2"));
  tests.push_back(std::make_pair("ala", "ala"));
  tests.push_back(std::make_pair("alabala", "alabala"));
  tests.push_back(std::make_pair("testing", "3"));
  tests.push_back(std::make_pair("telting", "3.1"));
  tests.push_back(std::make_pair("travel", "4"));
  tests.push_back(std::make_pair("tramway", "5"));
  tests.push_back(std::make_pair("tram", "6"));
  tests.push_back(std::make_pair("tramva", "7"));

  hash_transformation* hasher = new SHA256_cryptopp();
  state_impl state(hasher);

  // For each node added, check if the previous nodes are still correct
  for (uint32_t i = 0; i < tests.size(); i++) {
    state.set(tests[i].first, tests[i].second);
    for (uint32_t j = 0; j <= i; j++) {
      assert(state.get(tests[j].first) == tests[j].second);
    }
  }
}

void test_persistent_blobstore_create_mapped_file() {
  std::vector<uint64_t> ids;
  std::vector<std::string> data;
  data.push_back("data 1");
  data.push_back("data 2, data 2");
  data.push_back("data 3, data 3, data 3");
  data.push_back("data 4, data 4, data 4, data 4");
  data.push_back("data 5, data 5, data 5, data 5, data 5");
  data.push_back("data 6, data 6, data 6, data 6, data 6, data 6");
  data.push_back("data 7, data 7, data 7, data 7, data 7, data 7, data 7");
  data.push_back("data 8, data 8, data 8, data 8, data 8, data 8, data 8, data 8");
  data.push_back("data 9, data 9, data 9, data 9, data 9, data 9, data 9, data 9, data 9");
  {
    persistent_blobstore bs1;
    bs1.map_file("/tmp/mapped_file.txt");
    for (int i = 0; i < data.size(); i++) {
      ids.push_back(bs1.store(data[i].size(), reinterpret_cast<const uint8_t*>(data[i].data())));
    }
  }
  uint32_t sz;
  uint8_t* pData;
  persistent_blobstore bs1;
  bs1.map_file("/tmp/mapped_file.txt");
  for (int i = 0; i < data.size(); i++) {
    pData = bs1.get(ids[i], &sz);
    std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
    // assert(!std::memcmp(pData, &data[i][0], sz));
  }
  // for (int i = 0; i < 6; i++) {
  //   pData = bs1.get(ids[i], &sz);
  //   std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
  // }
}

void testnet_test_all() {
  node::register_node_type("lua", [](const std::string& id, const std::string& proto_id)->std::shared_ptr<node> {
    return std::shared_ptr<node>(new lua_node(id, proto_id));
  });

  boost::filesystem::path full_path(boost::filesystem::current_path());
  std::cout << "Current path is : " << full_path << std::endl;

  assert(smart_protocol::load("chat", "../../../../../../tests/testnet/testproto/"));

  std::shared_ptr<automaton::core::network::simulation> sim = automaton::core::network::simulation::get_simulator();
  sim->simulation_start(50);

  assert(testnet::create_testnet("lua", "testnet", "chat", testnet::network_protocol_type::simulation, 5,
    {
      {1, {2, 3}}, {2, {4, 5}}
    }));

  bool worker_running = true;
  auto net = testnet::get_testnet("testnet");

  std::thread worker_thread = std::thread([&]() {
    while (worker_running) {
      std::vector<std::string> node_ids = net->list_nodes();
      for (uint32_t i = 0; i < node_ids.size(); ++i) {
        if (!worker_running) {
          break;
        }
        auto node = node::get_node(node_ids[i]);
        if (node != nullptr) {
          uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
          if (current_time >= (node->get_time_to_update())) {
            node->process_update(current_time);
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  auto msg_factory = smart_protocol::get_protocol("chat")->get_factory();
  auto msg1 = msg_factory->new_message_by_name("Peers");
  auto msg2 = msg_factory->new_message_by_name("Peers");

  // There should be only one testnet named "testnet"
  assert(testnet::list_testnets().size() == 1);
  assert(testnet::list_testnets()[0] == "testnet");

  // Wait until all nodes receive the message
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));

  // Node "testnet_4" should have only one peer - "testnet_2"
  std::string result1 = node::get_node("testnet_4")->process_cmd("get_peers", "");
  msg1->deserialize_message(result1);
  assert(msg1->get_repeated_field_size(1) == 1);
  assert(msg1->get_repeated_blob(1, 0) == "testnet_2");

  std::string result2 = node::get_node("testnet_3")->process_cmd("get_heard_of", "");
  msg2->deserialize_message(result2);
  // Nodes should have heard of all 5 peers
  assert(msg2->get_repeated_field_size(1) == 5);

  // Sort nodes names
  std::set<std::string> result;
  for (uint32_t i = 0; i < 5; ++i) {
    result.insert(msg2->get_repeated_blob(1, i));
  }
  auto it = result.begin();
  for (uint32_t i = 1; it != result.end() && (i <= 5); ++it, ++i) {
    assert(*it == ("testnet_" + std::to_string(i)));
  }

  worker_running = false;
  worker_thread.join();
  net = nullptr;

  testnet::destroy_testnet("testnet");
  assert(testnet::list_testnets().size() == 0);
  assert(node::list_nodes().size() == 0);

  sim->simulation_stop();
}

void test_data_protobufs() {
  const char* TEST_MSG = "TestMsg5";
  const char* MANY_FIELDS_PROTO = R"(syntax = "proto3";

  message TestMsg {
  }

  message TestMsg2 {
    int32 a = 10;
    int32 l = 2;
    string o = 4;

    int32 b = 1;
    int32 c = 3;
    string d = 5;
    message TestMsg3 {
      repeated string s = 2;
    }
    repeated int32 k = 6;
  }

  message TestMsg4 {
    int32 a = 10;
    int32 l = 2;
    string o = 4;
    message TestMsg5 {
      repeated string s = 2;
      message TestMsg6 {
        repeated string s = 2;
      }
    }
    repeated int32 k = 3;
  }

  message TestMsg5 {
    int32 a = 1;
    uint32 b = 2;
    string c = 3;
    bytes d = 4;
    fixed32 e = 5;
    sint64 f = 6;
    repeated bool g = 7;
  })";

  protobuf_factory pb_factory;
  protobuf_schema loaded_schema(MANY_FIELDS_PROTO);
  pb_factory.import_schema(&loaded_schema, "test", "");
  int k;
  int id = pb_factory.get_schema_id(TEST_MSG);
  k = pb_factory.get_fields_number(id);
  std::string type;
  type = pb_factory.get_field_type(id, 1);
  type = pb_factory.get_field_type(id, 2);
  type = pb_factory.get_field_type(id, 3);
  type = pb_factory.get_field_type(id, 4);
  type = pb_factory.get_field_type(id, 5);
  type = pb_factory.get_field_type(id, 6);
  type = pb_factory.get_field_type(id, 7);
}

struct hash_test {
  const char* hash_function;
  const char* input;
  const char* output;
};

void test_script_hash_functions() {
  hash_test tests[] = {
    {"keccak256", "", "C5D2460186F7233C927E7DB2DCC703C0E500B653CA82273B7BFAD8045D85A470"},
    {"keccak256", "abc", "4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45"},
    {"ripemd160", "", "9C1185A5C5E9FC54612808977EE8F548B2258D31"},
    {"sha256", "", "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"},
    {"sha3", "", "A7FFC6F8BF1ED76651C14756A061D662F580FF4DE43B49FA82D80A4B80F8434A"},
    {"sha512", "",
      "CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715DC83F4A921D36CE9CE"
      "47D0D13C5D85F2B0FF8318D2877EEC2F63B931BD47417A81A538327AF927DA3E"},
  };

  std::shared_ptr<protobuf_factory> data_factory;
  automaton::core::script::engine lua(data_factory);
  lua.bind_core();

  for (auto test : tests) {
    auto result = lua[test.hash_function](test.input);
    assert(automaton::core::io::bin2hex(result) == test.output);
  }
}

//==============================================================================
MainComponent::MainComponent() {
  setSize(600, 400);

  test_script_hash_functions();
  test_state_impl_set_and_get();
  test_persistent_blobstore_create_mapped_file();
  // testnet_test_all();

  // automaton::core::network::tcp_init();
}

MainComponent::~MainComponent() {
}

//==============================================================================
void MainComponent::paint(Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

  g.setFont(Font(16.0f));
  g.setColour(Colours::white);
  g.drawText("Automaton Playground", getLocalBounds(), Justification::centred, true);
}

void MainComponent::resized() {
  // This is called when the MainComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.
}
