#include "automaton/core/eth_contract/eth_contract_raw.h"

#include <iomanip>
#include <string>
#include <utility>

#include <json.hpp>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/io/io.h"
#include "automaton/core/network/tcp_implementation.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::io::bin2hex;
using automaton::core::network::connection_id;
using automaton::core::network::connection;

using json = nlohmann::json;

static const char* CRLF = "\r\n";
static const char* CRLF2 = "\r\n\r\n";
static const uint32_t BUFFER_SIZE = 512;
static const uint32_t WAITING_HEADER = 1;
static const uint32_t WAITING_BODY = 2;
static const uint32_t GAS_LIMIT = 6000000;  // TODO(kari): Get this value from the blockchain before sending transaction

namespace automaton {
namespace core {
namespace eth_contract {

std::string hash(const std::string& data) {
  Keccak_256_cryptopp hasher;
  uint8_t digest[32];
  hasher.calculate_digest(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  return std::string(reinterpret_cast<char*>(digest), 32);
}

std::string dec_to_32hex(uint32_t n) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(64) << std::hex << n;
  return ss.str();
}

uint32_t hex_to_dec(const std::string& hex) {
  std::stringstream ss(hex);
  uint32_t n;
  ss >> std::hex >> n;
  return n;
}

std::unordered_map<std::string, std::shared_ptr<eth_contract> > eth_contract::contracts;

void eth_contract::register_contract(const std::string& server, const std::string& address,
    std::unordered_map<std::string, std::pair<std::string, bool> > signs) {
  if (contracts.find(address) != contracts.end()) {
    LOG(INFO) << "Contract already registered!";
    return;
  }
  contracts[address] = std::shared_ptr<eth_contract>(new eth_contract(server, address, signs));
}

std::shared_ptr<eth_contract> eth_contract::get_contract(const std::string& address) {
  auto it = contracts.find(address);
  if (it == contracts.end()) {
    LOG(ERROR) << "No contract with address " << address;
    return nullptr;
  }
  return it->second;
}

eth_contract::eth_contract(const std::string& server, const std::string& address,
    std::unordered_map<std::string, std::pair<std::string, bool> > signs):
    call_id(0), server(server), address(address) {
  for (auto it = signs.begin(); it != signs.end(); ++it) {
    signatures[it->first] = std::make_pair("0x" + (bin2hex(hash(it->second.first))).substr(0, 8), it->second.second);
  }

  header = "";
  body = "";
  message = "";

  buffer_size = BUFFER_SIZE;
  buffer = std::shared_ptr<char>(new char[buffer_size], std::default_delete<char[]>());;
}

eth_contract::~eth_contract() {}

void eth_contract::call(const std::string& from_address, const std::string& fname, const std::string& params,
    std::function<void(const automaton::core::common::status&, const std::string&)> callback) {
  auto it = signatures.find(fname);
  if (it == signatures.end()) {
    callback(automaton::core::common::status::invalid_argument("Function signature is not found!"), "");
    return;
  }
  callbacks[++call_id] = callback;
  bool is_transaction = it->second.second;
  std::stringstream data;
  data << "{\"jsonrpc\":\"2.0\",\"method\":\"" << (is_transaction ? "eth_sendTransaction" : "eth_call") <<
      "\",\"params\":[{ \"to\":\"" << address <<
      "\",\"data\":\"" << it->second.first << params <<
      "\",\"gas\":" << GAS_LIMIT <<  // TODO(kari): Get this from the blockchain
      ",\"from\":\"" << from_address << "\"}" << (is_transaction ? "" : ",\"latest\"") <<
      "],\"id\":" << call_id << "}";

  std::stringstream ss;
  ss << "POST / HTTP/1.1\r\n";
  ss << "Accept: */*\r\n";
  ss << "Content-Type: application/x-www-form-urlencoded\r\n";
  ss << "Content-Length: " << data.str().size() << "\r\n";
  ss << "\r\n" << data.str();

  if (conn == nullptr) {
    conn = connection::create("tcp", 1, server, shared_from_this());
    if (conn->init()) {
      LOG(DBUG) << "Connection init was successful!";
    } else {
      conn = nullptr;
      for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
        it->second(automaton::core::common::status::internal("Could not connect to server!"), "");
      }
      callbacks.clear();
      return;
    }
  }
  if (conn->get_state() == connection::state::disconnected) {
    conn->connect();
    conn->async_read(buffer, buffer_size, 0, WAITING_HEADER);
  }

  conn->async_send(ss.str(), call_id);
  std::cout << "\n======= REQUEST =======\n" << ss.str() << "\n=====================\n\n";
}

void eth_contract::handle_message(const automaton::core::common::status& s) {
  if (s.code != automaton::core::common::status::OK) {
    // TODO(kari): Send error to all callbakcs or only to the first?
    message = "";
    return;
  }

  json j;
  uint32_t call_id;
  std::stringstream ss(message);
  ss >> j;

  if (j.find("id") != j.end()) {
    call_id = j["id"].get<uint32_t>();
  } else {
    LOG(ERROR) << "Id not found!";
    message = "";
    return;
  }

  auto it = callbacks.find(call_id);
  if (it == callbacks.end()) {
    LOG(ERROR) << "Callback with id" << call_id << "not found!";
    message = "";
    return;
  }

  auto callback = it->second;

  if (j.find("error") != j.end()) {
    json obj = j["error"];
    std::string error = obj["message"].get<std::string>();
    callback(automaton::core::common::status::internal(error), "");
  } else if (j.find("result") != j.end()) {
    std::string result = j["result"].get<std::string>();
    callback(automaton::core::common::status::ok(), result.substr(2));
  }
  callbacks.erase(it);
  message = "";
  read_header();
}

void eth_contract::read_header() {
  uint32_t pos = header.find(CRLF2);
  if (pos == -1) {
    conn->async_read(buffer, buffer_size, 0, WAITING_HEADER);
  } else if (pos + 4 < header.size()) {
    body = header.substr(pos + 4);
    header.erase(pos + 4);
    read_header();
  } else {
    std::stringstream ss(header);
    std::string line;
    std::getline(ss >> std::ws, line);
    header = "";
    if (line.compare("HTTP/1.1 200 OK\r") != 0) {
      message = "";
      handle_message(automaton::core::common::status::aborted(line));
    } else {
      read_body();
    }
  }
}

void eth_contract::read_body() {
  uint32_t pos = body.find(CRLF);
  if (pos == -1) {
    conn->async_read(buffer, buffer_size, 0, WAITING_BODY);
    return;
  }

  uint32_t expected = hex_to_dec(body.substr(0, pos));  // TODO(kari): Could have spaces and other data. Handle it.

  if (expected == 0) {  // If we are not expecting any more chunks/bytes, the rest of the data starts with a header
    if (pos + 2 < body.size()) {
      header = body.substr(pos + 2);
    } else {
      header = "";
    }
    handle_message(automaton::core::common::status::ok());
  } else {  // Chunk size > 0 so we are expecting more data
    uint32_t pos2 = body.find(CRLF, pos + 2);  // the end of the chunk
    if (pos2 == -1) {  // If the end of the chunk has not yet arrived, read more data
      conn->async_read(buffer, buffer_size, 0, WAITING_BODY);
    } else {
      // Substract the whole chunk(message part) and remove it from body as it is already handled.
      message += body.substr(pos + 2, pos2 - pos - 2);
      body.erase(0, pos2 + 2);
      read_body();
    }
  }
}

void eth_contract::on_message_received(connection_id c, std::shared_ptr<char> buffer,
    uint32_t bytes_read, uint32_t mid) {
  std::string s(buffer.get(), bytes_read);
  std::cout << "=== RECEIVED ===\n" << s << "\n====" << std::endl;
  switch (mid) {
    case WAITING_HEADER: {
      header += s;
      read_header();
    }
    break;
    case WAITING_BODY: {
      body += s;
      read_body();
    }
    break;
  }
}

void eth_contract::on_message_sent(connection_id c, uint32_t mid, const automaton::core::common::status& s) {
  LOG(INFO) << "Message " << mid << " was sent. Status: " << s;
}

void eth_contract::on_connected(connection_id c) {
  LOG(INFO) << "Connected";
}

void eth_contract::on_disconnected(connection_id c) {
  LOG(INFO) << "Disconnected";
}

void eth_contract::on_connection_error(connection_id c, const automaton::core::common::status& s) {
  if (s.code == automaton::core::common::status::OK) {
    return;
  }
  LOG(ERROR) << s;
  callbacks.clear();
}

}  // namespace eth_contract
}  // namespace core
}  // namespace automaton
