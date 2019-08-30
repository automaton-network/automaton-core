#include "automaton/core/eth_contract/eth_contract0.h"

#include <iomanip>
#include <string>
#include <utility>

#include <json.hpp>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/io/io.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::io::bin2hex;

using json = nlohmann::json;

static const char* GAS_LIMIT = "0x5B8D80";  // TODO(kari): Get this value from the blockchain before sending transaction

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

std::string create_raw_transaction(const std::string& tx) {
  return "";
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
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, server.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &message);

    // DEBUG
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
  }
}

eth_contract::~eth_contract() {
  if (curl) {
    curl_easy_cleanup(curl);
  }
}

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
  std::string string_data;
  if (!is_transaction) {
    data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{ \"to\":\"" << address <<
        "\",\"data\":\"" << it->second.first << params <<
        "\",\"gas\":\"" << GAS_LIMIT << "\"},\"latest\"" << "],\"id\":" << call_id << "}";
  } else {
    callbacks[call_id](automaton::core::common::status::internal(
        "eth_sendTransaction and eth_sendRawTransaction are not yet supported!!"), "");
    callbacks.erase(call_id);
    return;
    // send raw transaction
    // std::stringstream ss;
    // ss << "{\"from\":\"" << from_address << "\", \"to\":\"" << address << "\", \"data\":\"" << params << "\"}";
    // std::string signed_tx = create_raw_transaction(ss.str());
    //
    // data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[{\"data\":\"" <<
    // signed_tx << "},\"latest\"") << "],\"id\":" << call_id << "}";
  }

  string_data = data.str();
  if (curl) {
    /* set the error buffer as empty before performing a request */
    curl_err_buf[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, string_data.c_str());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      size_t len = strlen(curl_err_buf);
      LOG(ERROR) << "Curl result code != CURLE_OK. Result code: " << res;
      if (len) {
        LOG(ERROR) << "Error message: " << curl_err_buf;
      }
    } else {
      handle_message(automaton::core::common::status::ok());
    }
  } else {
    LOG(ERROR) << "No curl!";
    callbacks[call_id](automaton::core::common::status::internal("No curl!"), "");
    callbacks.erase(call_id);
  }

  LOG(INFO) << "\n======= REQUEST =======\n" << string_data << "\n=====================";
}

void eth_contract::handle_message(const automaton::core::common::status& s) {
  if (s.code != automaton::core::common::status::OK) {
    // TODO(kari): Send error to all callbakcs or only to the first?
    LOG(ERROR) << "Call result:: status code -> " << s << " message: " << message;
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
}

size_t eth_contract::curl_callback(void *contents, size_t size, size_t nmemb, std::string *s) {
  size_t new_length = size * nmemb;
  try {
    s->append(reinterpret_cast<char*>(contents), new_length);
    LOG(INFO) << "=== CHUNK ===\n"
        << std::string(reinterpret_cast<char*>(contents), new_length) << "\n ===== EoCH ====";
  }
  catch (std::bad_alloc &e) {
    LOG(ERROR) << "Bad_alloc while reading data!";
    return 0;
  }
  return new_length;
}

}  // namespace eth_contract
}  // namespace core
}  // namespace automaton
