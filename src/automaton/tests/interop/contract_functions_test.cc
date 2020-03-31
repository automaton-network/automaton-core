#include <fstream>
#include <iostream>

#include <json.hpp>

#include "automaton/core/interop/ethereum/eth_helper_functions.h"
#include "automaton/core/interop/ethereum/eth_contract_curl.h"
#include "automaton/core/io/io.h"

using json = nlohmann::json;

using namespace automaton::core::interop::ethereum;  // NOLINT

static const char* JSON_FILE = "../contracts/koh/build/contracts/KingAutomaton.json";
// Ganache
static const char* URL = "127.0.0.1:7545";
static const char* CONTRACT_ADDR = "0x22D9d6faB361FaA969D2EfDE420472633cBB7B11";
// static const char* ADDRESS = "0x603CB0d1c8ab86E72beb3c7DF564A36D7B85ecD2";
// static const char* PRIVATE_KEY = "56aac550d97013a8402c98e3b2aeb20482d19f142a67022d2ab357eb8bb673b0";

int main() {
  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  curl_global_init(CURL_GLOBAL_ALL);

  using namespace automaton::core::interop::ethereum;  // NOLINT

  std::fstream fs(JSON_FILE, std::fstream::in);
  json j, abi;
  fs >> j;
  if (j.find("abi") != j.end()) {
    abi = j["abi"].get<json>();
  } else {
    LOG(FATAL) << "No abi!";
  }
  std::string abi_string = abi.dump();
  eth_contract::register_contract(URL, CONTRACT_ADDR, abi_string);
  fs.close();

  auto contract = eth_contract::get_contract(CONTRACT_ADDR);
  if (contract == nullptr) {
    LOG(ERROR) << "Contract is NULL";
    curl_global_cleanup();
    return 0;
  }

  status s = status::ok();

  s = contract->call("getOwners", "[0,16]");
  if (s.code == automaton::core::common::status::OK) {
    std::vector<std::string> res;
    j = json::parse(s.msg);
    try {
      res = (*j.begin()).get<std::vector<std::string> >();
    } catch (const std::exception& e) {
      std::cout << "JSON Error! " << e.what() << std::endl;
    }
    for (uint32_t i = 0; i < res.size(); ++i) {
      std::cout << "owner " << i << ": " << res[i] << std::endl;
    }
  } else {
    std::cout << "Error (getOwners()) " << s << std::endl;
  }

  s = contract->call("getDifficulties", "[15,5]");
  if (s.code == automaton::core::common::status::OK) {
    std::vector<std::string> res;
    j = json::parse(s.msg);
    try {
      res = (*j.begin()).get<std::vector<std::string> >();
    } catch (const std::exception& e) {
      std::cout << "JSON Error! " << e.what() << std::endl;
    }
    for (uint32_t i = 0; i < res.size(); ++i) {
      std::cout << "difficulty " << i << ": " << bin2hex(dec_to_i256(false, res[i])) << std::endl;
    }
  } else {
    std::cout << "Error (getDifficulties()) " << s << std::endl;
  }

  s = contract->call("getLastClaimTimes", "[2,18]");
  if (s.code == automaton::core::common::status::OK) {
    std::vector<std::string> res;
    j = json::parse(s.msg);
    try {
      res = (*j.begin()).get<std::vector<std::string> >();
    } catch (const std::exception& e) {
      std::cout << "JSON Error! " << e.what() << std::endl;
    }
    for (uint32_t i = 0; i < res.size(); ++i) {
      std::cout << "last claimed " << i << ": " << res[i] << std::endl;
    }
  } else {
    std::cout << "Error (getLastClaimTimes()) " << s << std::endl;
  }

  return 0;
}
