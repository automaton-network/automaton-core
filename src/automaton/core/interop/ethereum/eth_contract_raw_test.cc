#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "automaton/core/interop/ethereum/eth_contract_raw.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"

using automaton::core::eth_contract::dec_to_32hex;
using automaton::core::eth_contract::hex_to_dec;
using automaton::core::network::connection_id;
using automaton::core::network::connection;

/*
  TODO(kari):
  * test chunks with huge response
  * make this file test
*/

const char* CONTRACT_ADDR = "0x22D9d6faB361FaA969D2EfDE420472633cBB7B11";
const char* MY_ADDR = "0x603CB0d1c8ab86E72beb3c7DF564A36D7B85ecD2";
static const char* SERVER_IP = "127.0.0.1:7545";

void callback_func(const automaton::core::common::status& s, const std::string& response) {
  std::cout << "\n======= RESPONSE =======\n" << s << "\n" << response << "\n=====================\n\n";
}

int main() {
  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  using namespace automaton::core::eth_contract;  // NOLINT

  automaton::core::network::tcp_init();

  eth_contract::register_contract(SERVER_IP, CONTRACT_ADDR, {
    {"getSlotsNumber", {"getSlotsNumber()", false}},
    {"getSlotOwner", {"getSlotOwner(uint256)", false}},
    {"getSlotDifficulty", {"getSlotDifficulty(uint256)", false}},
    {"getSlotLastClaimTime", {"getSlotLastClaimTime(uint256)", false}},
    {"getMask", {"getMask()", false}},
    {"getClaimed", {"getClaimed()", false}},
    {"claimSlot", {"claimSlot(bytes32,bytes32,uint8,bytes32,bytes32)", true}}
  });

  auto contract = eth_contract::get_contract(CONTRACT_ADDR);
  if (contract == nullptr) {
    LOG(ERROR) << "Contract is NULL";
    return 0;
  }

  contract->call(MY_ADDR, "getSlotsNumber", "",
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Number of slots: " << hex_to_dec(response) << std::endl;
        } else {
          std::cout << "Error (getSlotsNumber()) " << s << std::endl;
        }
      });
  contract->call(MY_ADDR, "getSlotOwner", dec_to_32hex(2),
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Slot 2 owner: " << response << std::endl;
        } else {
          std::cout << "Error (getSlotOwner(2)) " << s << std::endl;
        }
      });
  contract->call(MY_ADDR, "getSlotDifficulty", dec_to_32hex(2),
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Slot 2 difficulty: " << response << std::endl;
        } else {
          std::cout << "Error (getSlotDifficulty(2)) " << s << std::endl;
        }
      });
  contract->call(MY_ADDR, "getSlotLastClaimTime", dec_to_32hex(2),
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Slot 2 last claim time: " << hex_to_dec(response) << std::endl;
        } else {
          std::cout << "Error (getSlotLastClaimTime(2)) " << s << std::endl;
        }
      });
  contract->call(MY_ADDR, "getMask", "",
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Mask: " << response << std::endl;
        } else {
          std::cout << "Error (getMask()) " << s << std::endl;
        }
      });
  contract->call(MY_ADDR, "getClaimed", "",
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Number of slot claims: " << hex_to_dec(response) << " / expected 0" << std::endl;
        } else {
          std::cout << "Error (getClaimed()) " << s << std::endl;
        }
      });

  std::stringstream ss;
  ss << "8a3892c2e85cf7fdbd795f2e91e88e406bad72b5a40d3511d64f9e4b57417477" <<
      "83f7b3e3c13b106102fd5cf8c41e6950d6f14b28391189c5fb1400cc0336a391" << std::string(62, '0') << "1c" <<
      "87c600b5e492f852a057e391ed4cd4c4e07b10c959777939e195cdc84cb9c434" <<
      "306859f991a82dc312fafa31b13bb182e26148e479bae608edd55090cf0e30e2";

  contract->call(MY_ADDR, "claimSlot", ss.str(),
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Slot claimed! " << response << std::endl;
        } else {
          std::cout << "Error (claimSlot) " << s << std::endl;
        }
      });
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  contract->call(MY_ADDR, "getClaimed", "",
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Number of slot claims: " << hex_to_dec(response) << " / expected 1" << std::endl;
        } else {
          std::cout << "Error (getClaimed()) " << s << std::endl;
        }
      });

  std::stringstream ss2;
  ss2 << std::string(62 + 64 + 64, '0') << "1B" <<
      "6364E1C27885E23AA3F79614EC3ABB11F6FBA395FF854B2F8B153783C79256CA" <<
      "45BC81CF0B36CF73FCB47079489AC79A10BC50D0BECF590A384D6ACAA03C5DCD";

  contract->call(MY_ADDR, "claimSlot", ss2.str(),
      [](const automaton::core::common::status& s, const std::string& response) {
        if (s.code == automaton::core::common::status::OK) {
          std::cout << "Error expected but got OK! " << hex_to_dec(response) << std::endl;
        } else {
          std::cout << "Expected error in claimSlot(...) " << s << std::endl;
        }
      });

  std::this_thread::sleep_for(std::chrono::milliseconds(6000));

  automaton::core::network::tcp_release();
  return 0;
}
