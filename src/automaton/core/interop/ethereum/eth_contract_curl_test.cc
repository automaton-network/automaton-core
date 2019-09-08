#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <curl/curl.h>  // NOLINT

#include "automaton/core/interop/ethereum/eth_contract_curl.h"
#include "automaton/core/io/io.h"

using automaton::core::interop::ethereum::dec_to_32hex;
using automaton::core::interop::ethereum::hex_to_dec;
using automaton::core::common::status;

/*
  TODO(kari):
  * make this file test
*/

void callback_func(const automaton::core::common::status& s, const std::string& response) {
  std::cout << "\n======= RESPONSE =======\n" << s << "\n" << response << "\n=====================\n\n";
}

int main() {
  std::string url, contract_addr;
  std::cout << "Enter URL: ";
  std::cin >> url;
  std::cout << "Enter contract address: ";
  std::cin >> contract_addr;

  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  curl_global_init(CURL_GLOBAL_ALL);

  using namespace automaton::core::interop::ethereum;  // NOLINT

  eth_contract::register_contract(url, contract_addr, {
    {"getSlotsNumber", {"getSlotsNumber()", false}},
    {"getSlotOwner", {"getSlotOwner(uint256)", false}},
    {"getSlotDifficulty", {"getSlotDifficulty(uint256)", false}},
    {"getSlotLastClaimTime", {"getSlotLastClaimTime(uint256)", false}},
    {"getMask", {"getMask()", false}},
    {"getClaimed", {"getClaimed()", false}},
    {"claimSlot", {"claimSlot(bytes32,bytes32,uint8,bytes32,bytes32)", true}}
  });

  auto contract = eth_contract::get_contract(contract_addr);
  if (contract == nullptr) {
    LOG(ERROR) << "Contract is NULL";

    curl_global_cleanup();
    return 0;
  }

  status s = status::ok();

  s = contract->call("", "getSlotsNumber", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Number of slots: " << hex_to_dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getSlotsNumber()) " << s << std::endl;
  }

  s = contract->call("", "getSlotOwner", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 owner: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getSlotOwner(2)) " << s << std::endl;
  }

  s = contract->call("", "getSlotDifficulty", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 difficulty: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getSlotDifficulty(2)) " << s << std::endl;
  }

  s = contract->call("", "getSlotLastClaimTime", dec_to_32hex(2));
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Slot 2 last claim time: " << hex_to_dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getSlotLastClaimTime(2)) " << s << std::endl;
  }

  s = contract->call("", "getMask", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Mask: " << s.msg << std::endl;
  } else {
    std::cout << "Error (getMask()) " << s << std::endl;
  }

  s = contract->call("", "getClaimed", "");
  if (s.code == automaton::core::common::status::OK) {
    std::cout << "Number of slot claims: " << hex_to_dec(s.msg) << std::endl;
  } else {
    std::cout << "Error (getClaimed()) " << s << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(6000));

  curl_global_cleanup();
  return 0;
}
