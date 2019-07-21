#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "automaton/core/eth_contract/eth_contract.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"

using automaton::core::eth_contract::dec_to_32hex;
using automaton::core::eth_contract::hex_to_dec;
using automaton::core::network::connection_id;
using automaton::core::network::connection;

/*
  TODO(kari):
  * claimSlot is not working correctly
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
    {"getSlotsNumber", "getSlotsNumber()"},
    {"getSlotOwner", "getSlotOwner(uint256)"},
    {"getSlotDifficulty", "getSlotDifficulty(uint256)"},
    {"getSlotLastClaimTime", "getSlotLastClaimTime(uint256)"},
    {"getMask", "getMask()"},
    {"getClaimed", "getClaimed()"},
    {"claimSlot", "claimSlot(bytes32,bytes32,uint8,bytes32,bytes32)"}
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
  ss << "8A386B8975A517CDB68D19D6FE140B51C396AFDC8ED2FA42BD49C1557A2CE94B" <<
      "CCC3328249603C523158E689423B9B3B7467F9520FB6A930A0D04E8369DDF527" << std::string(62, '0') << "1B" <<
      "6364E1C27885E23AA3F79614EC3ABB11F6FBA395FF854B2F8B153783C79256CA" <<
      "45BC81CF0B36CF73FCB47079489AC79A10BC50D0BECF590A384D6ACAA03C5DCD";

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

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  automaton::core::network::tcp_release();
  return 0;
}
