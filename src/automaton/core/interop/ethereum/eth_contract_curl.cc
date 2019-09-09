#include "automaton/core/interop/ethereum/eth_contract_curl.h"

#include <iomanip>
#include <string>
#include <utility>

#include <json.hpp>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/io/io.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::io::bin2hex;
using automaton::core::common::status;

using json = nlohmann::json;

static const char* GAS_LIMIT = "0x5B8D80";  // TODO(kari): Get this value from the blockchain before sending transaction

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

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
    curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");

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

status eth_contract::call(const std::string& from_address, const std::string& fname, const std::string& params) {
  call_id++;
  auto it = signatures.find(fname);
  if (it == signatures.end()) {
    return status::invalid_argument("Function signature is not found!");
  }
  bool is_transaction = it->second.second;
  std::stringstream data;
  std::string string_data;
  if (!is_transaction) {
    data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{ \"to\":\"" << address <<
        "\",\"data\":\"" << it->second.first << params <<
        "\",\"gas\":\"" << GAS_LIMIT << "\"},\"latest\"" << "],\"id\":" << call_id << "}";
  } else {
    return status::internal("eth_sendTransaction and eth_sendRawTransaction are not yet supported!!");

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
      return status::internal("CURL error");
    } else {
      return handle_message();
    }
  } else {
    LOG(ERROR) << "No curl!";
    return status::internal("No curl!");
  }

  LOG(INFO) << "\n======= REQUEST =======\n" << string_data << "\n=====================";
}

status eth_contract::handle_message() {
  json j;
  uint32_t result_call_id;
  std::stringstream ss(message);
  ss >> j;
  message = "";

  if (j.find("id") != j.end()) {
    result_call_id = j["id"].get<uint32_t>();
  } else {
    LOG(ERROR) << "ID not found!";
    return status::internal("ID not found!");
  }

  if (result_call_id != call_id) {
    LOG(ERROR) << "Result ID " << result_call_id << " does not match request ID: " << call_id;
    return status::internal("Result ID does not match request ID!");
  }

  if (j.find("error") != j.end()) {
    json obj = j["error"];
    std::string error = obj["message"].get<std::string>();
    return status::internal(error);
  } else if (j.find("result") != j.end()) {
    std::string result = j["result"].get<std::string>();
    return status::ok(result.substr(2));
  }
  return status::internal("No result and no error!?");
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

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton
