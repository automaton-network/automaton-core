#include <future>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "base64.h"  // NOLINT
#include "filters.h"  // NOLINT
#include <json.hpp>

#include "automaton/core/cli/cli.h"
#include "automaton/core/data/factory.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/network/http_server.h"
#include "automaton/core/network/simulated_connection.h"
#include "automaton/core/network/tcp_implementation.h"
#include "automaton/core/node/lua_node/lua_node.h"
#include "automaton/core/node/node.h"
#include "automaton/core/node/node_updater.h"
#include "automaton/core/script/engine.h"
#include "automaton/core/smartproto/smart_protocol.h"
#include "automaton/core/testnet/testnet.h"

#include "automaton/core/io/io.h"  //  IO needs to be included after boost

using automaton::core::data::factory;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::data::schema;
using automaton::core::io::get_file_contents;
using automaton::core::network::http_server;
using automaton::core::node::luanode::lua_node;
using automaton::core::node::node;
using automaton::core::node::default_node_updater;
using automaton::core::script::engine;
using automaton::core::smartproto::smart_protocol;
using automaton::core::testnet::testnet;

using json = nlohmann::json;

using std::make_unique;
using std::string;
using std::unique_ptr;
using std::vector;

void string_replace(string* str,
                    const string& oldStr,
                    const string& newStr) {
  string::size_type pos = 0u;
  while ((pos = str->find(oldStr, pos)) != string::npos) {
     str->replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}

static const char* automaton_ascii_logo_cstr =
  "\n\x1b[40m\x1b[1m"
  "                                                                     " "\x1b[0m\n\x1b[40m\x1b[1m"
  "                                                                     " "\x1b[0m\n\x1b[40m\x1b[1m"
  "    @197m█▀▀▀█ @39m█ █ █ @11m▀▀█▀▀ @129m█▀▀▀█ @47m█▀█▀█ @9m█▀▀▀█ @27m▀▀█▀▀ @154m█▀▀▀█ @13m█▀█ █            " "\x1b[0m\n\x1b[40m\x1b[1m" // NOLINT
  "    @197m█▀▀▀█ @39m█ ▀ █ @11m█ █ █ @129m█ ▀ █ @47m█ ▀ █ @9m█▀▀▀█ @27m█ █ █ @154m█ ▀ █ @13m█ █ █   @15mCORE     " "\x1b[0m\n\x1b[40m\x1b[1m" // NOLINT
  "    @197m▀ ▀ ▀ @39m▀▀▀▀▀ @11m▀ ▀ ▀ @129m▀▀▀▀▀ @47m▀ ▀ ▀ @9m▀ ▀ ▀ @27m▀ ▀ ▀ @154m▀▀▀▀▀ @13m▀ ▀▀▀   @15mv0.0.1   " "\x1b[0m\n\x1b[40m\x1b[1m" // NOLINT
  "                                                                     " "\x1b[0m\n@0m"
  "▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀" "\x1b[0m\n";

class rpc_server_handler: public automaton::core::network::http_server::server_handler {
  engine* script;

 public:
    explicit rpc_server_handler(engine* en): script(en) {}
    std::string handle(std::string json_cmd, http_server::status_code* s) {
      std::stringstream sstr(json_cmd);
      nlohmann::json j;
      sstr >> j;
      std::string cmd = "rpc_";
      std::string msg = "";
      if (j.find("method") != j.end() && j.find("msg") != j.end()) {
        cmd += j["method"].get<std::string>();
        msg = j["msg"].get<std::string>();
      } else {
        LOG(ERROR) << "ERROR in rpc server handler: Invalid request";
        *s = http_server::status_code::BAD_REQUEST;
        return "";
      }
      std::string params = "";
      LOG(INFO) << "Server received command: " << cmd << " -> " << automaton::core::io::bin2hex(msg);
      if (msg.size() > 0) {
        CryptoPP::StringSource ss(msg, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(params)));
      }
      if ((*script)[cmd] == nullptr) {
        LOG(ERROR) << "ERROR in rpc server handler: Invalid request";
        *s = http_server::status_code::BAD_REQUEST;
        return "";
      }
      sol::protected_function_result pfr = (*script)[cmd](params);
      if (!pfr.valid()) {
        sol::error err = pfr;
        LOG(ERROR) << "ERROR in rpc server handler: " << err.what();
        *s = http_server::status_code::INTERNAL_SERVER_ERROR;
        return "";
      }
      std::string result = pfr;
      if (s != nullptr) {
        *s = http_server::status_code::OK;
      } else {
        LOG(ERROR) << "Status code variable is missing";
      }
      std::string encoded;
      CryptoPP::StringSource ss(reinterpret_cast<const unsigned char*>(result.c_str()), result.size(), true,
          new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded)));
      return encoded;
    }
};

int main(int argc, char* argv[]) {
  string automaton_ascii_logo(automaton_ascii_logo_cstr);
  string_replace(&automaton_ascii_logo, "@", "\x1b[38;5;");
  auto core_factory = std::make_shared<protobuf_factory>();
  node::register_node_type("lua", [](const std::string& id, const std::string& proto_id)->std::shared_ptr<node> {
      return std::shared_ptr<node>(new lua_node(id, proto_id));
    });

  default_node_updater* updater;
{
  automaton::core::io::init_logger();
  automaton::core::cli::cli cli;
  engine script(core_factory);
  script.bind_core();

  // Bind node::node class
  auto node_type = script.create_simple_usertype<lua_node>();

  node_type.set(sol::call_constructor,
    sol::factories(
    [&](const std::string& id, std::string proto) -> std::shared_ptr<lua_node> {
      return std::dynamic_pointer_cast<lua_node>(node::create("lua", id, proto));
    }));

  // Bind this node to its own Lua state.
  node_type.set("add_peer", &lua_node::add_peer);
  node_type.set("remove_peer", &lua_node::remove_peer);
  node_type.set("connect", &lua_node::connect);
  node_type.set("disconnect", &lua_node::disconnect);
  node_type.set("send", &lua_node::send_message);
  node_type.set("listen", &lua_node::set_acceptor);

  node_type.set("msg_id", &lua_node::find_message_id);
  node_type.set("new_msg", &lua_node::create_msg_by_id);
  node_type.set("send", &lua_node::send_message);

  node_type.set("dump_logs", &lua_node::dump_logs);

  node_type.set("script", [](lua_node& n, std::string command) -> std::string {
    std::promise<std::string> prom;
    std::future<std::string> fut = prom.get_future();
    n.script(command, &prom);
    std::string result = fut.get();
    return result;
  });

  node_type.set("get_id", &lua_node::get_id);
  node_type.set("get_protocol_id", &lua_node::get_protocol_id);
  node_type.set("get_address", [](lua_node& n) -> std::string {
    std::shared_ptr<automaton::core::network::acceptor> a = n.get_acceptor();
    if (a) {
      return a->get_address();
    }
    return "";
  });

  node_type.set("process_cmd", &lua_node::process_cmd);

  node_type.set("process_update", &lua_node::process_update);

  node_type.set("get_time_to_update", &lua_node::get_time_to_update);

  node_type.set("call", [](lua_node& n, std::string command) {
    n.script(command, nullptr);
  });

  node_type.set("known_peers", [](lua_node& n) {
    LOG(DEBUG) << "getting known peers... " << &n;
    // TODO(asen): need STL support for logger
    // LOG(DEBUG) << n.list_known_peers();
    return sol::as_table(n.list_known_peers());
  });

  node_type.set("peers", [](lua_node& n) {
    LOG(DEBUG) << "getting peers... " << &n;
    // TODO(asen): need STL support for logger
    // LOG(DEBUG) << n.list_connected_peers();
    return sol::as_table(n.list_connected_peers());
  });

  node_type.set("get_peer_address", [](lua_node& n, uint32_t pid) {
    return n.get_peer_info(pid).address;
  });

  script.set_usertype("node", node_type);

  // Bind node static functions

  script.set_function("list_nodes_as_table", [&](){
    return sol::as_table(node::list_nodes());
  });

  script.set_function("get_node", [&](const std::string& node_id) -> std::shared_ptr<lua_node> {
    return std::dynamic_pointer_cast<lua_node>(node::get_node(node_id));
  });

  script.set_function("launch_node", [&](std::string node_id, std::string protocol_id, std::string address) {
    LOG(INFO) << "launching node ... " << node_id << " on " << protocol_id << " @ " << address;
    node::launch_node("lua", node_id, protocol_id, address);
    updater->add_node(node_id);
    return "";
  });

  script.set_function("remove_node", &node::remove_node);

  // Bind testnet static functions

  script.set("testnet_create", [&](std::string id, std::string smart_protocol_id, std::string ntype,
      uint32_t number_nodes, std::unordered_map<uint32_t, std::vector<uint32_t> > peer_list) {
    bool success = false;
    if (ntype == "simulation") {
      success = testnet::create_testnet("lua", id, smart_protocol_id, testnet::network_protocol_type::simulation,
          number_nodes, peer_list);
    } else if (ntype == "localhost") {
      success = testnet::create_testnet("lua", id, smart_protocol_id, testnet::network_protocol_type::localhost,
          number_nodes, peer_list);
    }
    if (!success) {
      throw std::runtime_error("Testnet creation failed!");
    }
    std::shared_ptr<testnet> net = testnet::get_testnet(id);
    for (auto nid : net->list_nodes()) {
      updater->add_node(nid);
    }
  });

  script.set("testnet_destroy", &testnet::destroy_testnet);

  script.set("testnet_list_all", &testnet::list_testnets);

  script.set_function("connect_testnet_nodes",
      [&](std::string id, std::unordered_map<uint32_t, std::vector<uint32_t> > peers_list) {
    auto net = testnet::get_testnet(id);
    if (net == nullptr) {
      LOG(ERROR) << "No testnet with id " << id;
    } else {
      net->connect(peers_list);
    }
  });

  script.set_function("list_testnet_nodes", [&](std::string id) {
    auto net = testnet::get_testnet(id);
    if (net == nullptr) {
      LOG(ERROR) << "No testnet with id " << id;
    } else {
      return net->list_nodes();
    }
    return std::vector<std::string>();
  });

  script.set_function("get_testnet_node_id", [&](std::string testnet_id, uint32_t index) -> std::string {
    auto net = testnet::get_testnet(testnet_id);
    if (net == nullptr) {
      LOG(ERROR) << "No testnet with id " << testnet_id;
      return "";
    }
    std::vector<std::string> nodes = net->list_nodes();
    if (index < 1 || index > nodes.size()) {
      LOG(ERROR) << "No node with index " << index;
      return "";
    }
    return nodes[index - 1];
  });

  // end of testnet functions

  script.set_function("history_add", [&](std::string cmd){
    cli.history_add(cmd.c_str());
  });

  script.set_function("hints_add", [&](std::string cmd){
    cli.hints_add(cmd.c_str());
  });

  script.set_function("hints_clear", [&](){
    cli.hints_clear();
  });

  script.set_function("load_protocol", [&](std::string proto_id, std::string path){
    smart_protocol::load(proto_id, path);
  });

  automaton::core::network::tcp_init();

  std::shared_ptr<automaton::core::network::simulation> sim = automaton::core::network::simulation::get_simulator();
  sim->simulation_start(100);
  cli.print(automaton_ascii_logo.c_str());

  script.safe_script(get_file_contents("automaton/examples/smartproto/common/names.lua"));
  script.safe_script(get_file_contents("automaton/examples/smartproto/common/dump.lua"));
  script.safe_script(get_file_contents("automaton/examples/smartproto/common/network.lua"));
  script.safe_script(get_file_contents("automaton/examples/smartproto/common/connections_graph.lua"));
  script.safe_script(get_file_contents("automaton/examples/smartproto/common/show_states.lua"));

  std::unordered_map<std::string, std::pair<std::string, std::string> > rpc_commands;
  uint32_t rpc_port = 0;
  uint32_t updater_workers_number = 0;
  uint32_t updater_workers_sleep_time = 0;

  std::ifstream i("automaton/core/coreinit.json");
  if (!i.is_open()) {
    LOG(ERROR) << "coreinit.json could not be opened";
  } else {
    nlohmann::json j;
    i >> j;
    i.close();
    std::vector<std::vector<std::string>> protocols = j["protocols"];
    for (auto& p : protocols) {
      std::string pid = p[0];
      std::string path = p[1];
      script.safe_script(get_file_contents((path + "init.lua").c_str()));
      smart_protocol::load(pid, path);
    }
    script.set_function("get_core_supported_protocols", [&](){
      std::unordered_map<std::string, std::unordered_map<std::string, std::string> > protocols;
      for (std::string proto : smart_protocol::list_protocols()) {
        auto p = smart_protocol::get_protocol(proto);
        protocols[proto] = p->get_msgs_definitions();
        protocols[proto]["config"] = p->get_configuration_file();
      }
      return sol::as_table(protocols);
    });

    std::vector<std::string> rpc_protos = j["command_definitions"];
    for (auto& p : rpc_protos) {
      schema* rpc_schema = new protobuf_schema(get_file_contents(p.c_str()));
      script.import_schema(rpc_schema);
    }
    std::vector<std::string> rpc_luas = j["command_implementations"];
    for (auto& p : rpc_luas) {
      script.safe_script(get_file_contents(p.c_str()));
    }
    for (auto& c : j["commands"]) {
      rpc_commands[c["cmd"]] = std::make_pair(c["input"], c["output"]);
    }
    rpc_port = j["rpc_config"]["default_port"];

    updater_workers_number = j["updater_config"]["workers_number"];
    updater_workers_sleep_time = j["updater_config"]["workers_sleep_time"];
  }
  i.close();
  updater = new default_node_updater(updater_workers_number, updater_workers_sleep_time, std::set<std::string>());
  updater->start();

  // Start dump_logs thread.
  std::mutex logger_mutex;
  bool stop_logger = false;
  std::thread logger([&]() {
    while (!stop_logger) {
      // Dump logs once per second.
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));
      logger_mutex.lock();
      try {
        sol::protected_function_result pfr;
        pfr = script.safe_script(
          R"(for k,v in pairs(networks) do
              dump_logs(k)
            end
          )");
        if (!pfr.valid()) {
          sol::error err = pfr;
          std::cout << "\n" << err.what() << "\n";
          break;
        }
      } catch (std::exception& e) {
        LOG(FATAL) << "Exception in logger: " << e.what();
      } catch (...) {
        LOG(FATAL) << "Exception in logger";
      }
      logger_mutex.unlock();
    }
  });

  std::shared_ptr<automaton::core::network::http_server::server_handler> s_handler(new rpc_server_handler(&script));
  http_server rpc_server(rpc_port, s_handler);
  rpc_server.run();

  while (1) {
    auto input = cli.input("\x1b[38;5;15m\x1b[1m|A|\x1b[0m ");
    if (input == nullptr) {
      cli.print("\n");
      break;
    }

    string cmd{input};
    cli.history_add(cmd.c_str());

    logger_mutex.lock();
    sol::protected_function_result pfr = script.safe_script(cmd, &sol::script_pass_on_error);
    logger_mutex.unlock();

    if (!pfr.valid()) {
      sol::error err = pfr;
      LOG(ERROR) << "Error while executing command: " << err.what();
    }
  }

  rpc_server.stop();

  updater->stop();
  delete updater;

  stop_logger = true;
  logger.join();

  LOG(DEBUG) << "Destroying lua state & objects";

  sim->simulation_stop();
}

  LOG(DEBUG) << "tcp_release";

  automaton::core::network::tcp_release();

  LOG(DEBUG) << "tcp_release done.";

  return 0;
}
