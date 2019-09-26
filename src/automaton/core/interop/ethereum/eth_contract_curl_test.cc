#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <curl/curl.h>  // NOLINT
#include <json.hpp>

#include "automaton/core/interop/ethereum/eth_contract_curl.h"
#include "automaton/core/interop/ethereum/eth_transaction.h"
#include "automaton/core/io/io.h"

using automaton::core::interop::ethereum::dec_to_32hex;
using automaton::core::interop::ethereum::eth_transaction;
using automaton::core::io::hex2dec;
using automaton::core::common::status;

using json = nlohmann::json;

// Ganache test
static const char* URL = "127.0.0.1:7545";
static const char* CONTRACT_ADDR = "22D9d6faB361FaA969D2EfDE420472633cBB7B11";
static const char* PRIVATE_KEY = "56aac550d97013a8402c98e3b2aeb20482d19f142a67022d2ab357eb8bb673b0";
static const char* JSON_FILE = "../contracts/koh/build/contracts/KingAutomaton.json";
/*
  TODO(kari):
  * make this file test
*/

void callback_func(const automaton::core::common::status& s, const std::string& response) {
  std::cout << "\n======= RESPONSE =======\n" << s << "\n" << response << "\n=====================\n\n";
}

int main() {
  // std::string url, contract_addr;
  // std::cout << "Enter URL: ";
  // std::cin >> url;
  // std::cout << "Enter contract address: ";
  // std::cin >> contract_addr;

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
  // std::string file_content((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
  eth_contract::register_contract(URL, CONTRACT_ADDR, abi_string);
  fs.close();

  auto contract = eth_contract::get_contract(CONTRACT_ADDR);
  if (contract == nullptr) {
    LOG(ERROR) << "Contract is NULL";
    curl_global_cleanup();
    return 0;
  }

  status s = status::ok();

  s = contract->call("getSlotsNumber", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Number of slots: " << hex2dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getSlotsNumber()) " << s << std::endl;
  }

  s = contract->call("getSlotOwner", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 owner: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getSlotOwner(2)) " << s << std::endl;
  }

  s = contract->call("getSlotDifficulty", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 difficulty: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getSlotDifficulty(2)) " << s << std::endl;
  }

  s = contract->call("getSlotLastClaimTime", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 last claim time: " << hex2dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getSlotLastClaimTime(2)) " << s << std::endl;
  }

  s = contract->call("getMask", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Mask: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getMask()) " << s << std::endl;
  }

  s = contract->call("getClaimed", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Number of slot claims: " << hex2dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getClaimed()) " << s << std::endl;
  }

  // std::cout << "Enter private key: ";
  // std::cin >> private_key;

  std::stringstream claim_slot_data;
  claim_slot_data << "6b2c8c48" << "8a3892c2e85cf7fdbd795f2e91e88e406bad72b5a40d3511d64f9e4b57417477" <<
      "83f7b3e3c13b106102fd5cf8c41e6950d6f14b28391189c5fb1400cc0336a391" << std::string(62, '0') << "1c"
      "87c600b5e492f852a057e391ed4cd4c4e07b10c959777939e195cdc84cb9c434" <<
      "306859f991a82dc312fafa31b13bb182e26148e479bae608edd55090cf0e30e2";

  eth_transaction t;
  t.nonce = "04";
  t.gas_price = "1388";  // 5 000
  t.gas_limit = "5B8D80";  // 6M
  t.to = CONTRACT_ADDR;
  t.value = "";
  t.data = claim_slot_data.str();
  t.chain_id = "01";

  s = contract->call("claimSlot", t.sign_tx(PRIVATE_KEY));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Claim slot result: " << s.msg << std::endl;
  } else {
    std::cout << "Error (claimSlot) " << s << std::endl;
  }

  curl_global_cleanup();
  return 0;
}
