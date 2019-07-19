#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "automaton/core/eth_contract/eth_contract.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"

using automaton::core::network::connection;
using automaton::core::network::connection_id;
using automaton::core::eth_contract::dec_to_32hex;

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

  contract->call(MY_ADDR, "getSlotsNumber", "", callback_func);
  contract->call(MY_ADDR, "getSlotOwner", dec_to_32hex(2), callback_func);
  contract->call(MY_ADDR, "getSlotDifficulty", dec_to_32hex(2), callback_func);
  contract->call(MY_ADDR, "getSlotLastClaimTime", dec_to_32hex(2), callback_func);
  contract->call(MY_ADDR, "getMask", "", callback_func);
  contract->call(MY_ADDR, "getClaimed", "", callback_func);

  std::stringstream ss;
  ss << "FFFFA600B54D95B84FA8664DBCB5043886EE62DA5D89E59898B925AA43D7340A" <<
      "0x8224A32BAD366C1AA0A98C7329B96888E3CE1350DB8D65851EE3CB92C627EE5C" << std::string(62, '0') << "1C" <<
      "0x364020867EDB23CE0B91120E8649D60E3C230E36580EBA9CBE74C05C5F2E3974" <<
      "0x06A0DB8D9DC51BDE1154AE1F31133E2571BD5101F012DE04DC64C61F946CAC1F";

  contract->call(MY_ADDR, "claimSlot", ss.str(), callback_func);
  std::this_thread::sleep_for(std::chrono::milliseconds(8000));
  contract->call(MY_ADDR, "getClaimed", "", callback_func);

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  automaton::core::network::tcp_release();
  return 0;
}
