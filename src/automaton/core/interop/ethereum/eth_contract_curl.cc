#include "automaton/core/interop/ethereum/eth_contract_curl.h"

#include <iomanip>
#include <string>
#include <utility>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/interop/ethereum/eth_transaction.h"
#include "automaton/core/interop/ethereum/eth_helper_functions.h"
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

std::string dec_to_32hex(uint32_t n) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(64) << std::hex << n;
  return ss.str();
}

std::unordered_map<std::string, std::shared_ptr<eth_contract> > eth_contract::contracts;

void eth_contract::register_contract(const std::string& server, const std::string& address,
    const std::string& abi_json) {
  if (contracts.find(address) != contracts.end()) {
    LOG(INFO) << "Contract already registered!";
    return;
  }
  contracts[address] = std::shared_ptr<eth_contract>(new eth_contract(server, address, abi_json));
}

std::shared_ptr<eth_contract> eth_contract::get_contract(const std::string& address) {
  auto it = contracts.find(address);
  if (it == contracts.end()) {
    LOG(ERROR) << "No contract with address " << address;
    return nullptr;
  }
  return it->second;
}

void eth_contract::parse_abi(json json_abi) {
  for (json::iterator it = json_abi.begin(); it != json_abi.end(); ++it) {
    json jobj = it.value();
    std::string name;
    bool is_transaction;
    if ((jobj.find("type") != jobj.end() && jobj["type"].get<std::string>() == "function") ||
        jobj.find("type") == jobj.end()) {  // default value for "type" is "function"
      std::stringstream inputs;
      std::stringstream outputs;
      std::stringstream signature;
      if (jobj.find("name") != jobj.end()) {
        name = jobj["name"].get<std::string>();
        signature << name << '(';
      } else {
        LOG(FATAL) << "Function doesn't have \"name\" !";
      }
      if (jobj.find("constant") != jobj.end()) {
        is_transaction = !jobj["constant"].get<bool>();
      } else {
        LOG(FATAL) << "Function doesn't have \"constant\" !";
      }
      inputs << '[';
      if (jobj.find("inputs") != jobj.end()) {
        std::vector<json> v_inputs = jobj["inputs"].get<std::vector<json> >();
        for (auto i = 0; i < v_inputs.size(); ++i) {
          auto& inp = v_inputs[i];
          if (inp.find("type") != inp.end()) {
            auto s = inp["type"].get<std::string>();
            signature << s;
            inputs << '"' << s << '"';
            if (i < v_inputs.size() - 1) {
              signature << ',';
              inputs << ',';
            }
          } else {
            LOG(FATAL) << "Input doesn't have \"type\" !";
          }
        }
      } else {
        LOG(FATAL) << "Function doesn't have \"inputs\" !";
      }
      outputs << '[';
      if (jobj.find("outputs") != jobj.end()) {
        std::vector<json> v_outputs = jobj["outputs"].get<std::vector<json> >();
        for (auto i = 0; i < v_outputs.size(); ++i) {
          auto& out = v_outputs[i];
          if (out.find("type") != out.end()) {
            outputs << '"' << out["type"].get<std::string>() << '"';
            if (i < v_outputs.size() - 1) {
              outputs << ',';
            }
          } else {
            LOG(FATAL) << "Output doesn't have \"type\" !";
          }
        }
      } else {
        LOG(FATAL) << "Function doesn't have \"outputs\" !";
      }
      inputs << ']';
      signature << ')';
      outputs << ']';
      signatures[name] = std::make_pair("0x" + (bin2hex(hash(signature.str()))).substr(0, 8), is_transaction);
      function_inputs[name] = inputs.str();
      function_outputs[name] = outputs.str();
    }
  }
}

eth_contract::eth_contract(const std::string& server, const std::string& address,
    const std::string& abi_json_string):call_id(0), server(server), address(address) {
  json j;
  std::stringstream ss(abi_json_string);
  try {
    ss >> j;
  } catch (const std::exception& e) {
    LOG(FATAL) << "Invalid json! " << e.what();
  }
  abi = j;
  parse_abi(abi);
  curl = curl_easy_init();
  if (curl) {
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, server.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &message);
    curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    // DEBUG
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
  }
}

eth_contract::~eth_contract() {
  if (curl) {
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
  }
}

status eth_contract::call(const std::string& fname, const std::string& params) {
  call_id++;
  auto it = signatures.find(fname);
  if (it == signatures.end()) {
    return status::invalid_argument("Function signature is not found!");
  }
  bool is_transaction = it->second.second;
  std::stringstream data;
  std::string string_data;
  if (!is_transaction) {
    auto p_it = function_inputs.find(fname);
    if (p_it == function_inputs.end()) {
      return status::invalid_argument("Function signature is not found in function_inputs!");
    }
    std::string encoded_params = "";
    if (p_it->second != "[]") {
      encoded_params = encode(p_it->second, params);
    }
    data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{ \"to\":\"" << address <<
        "\",\"data\":\"" << it->second.first << bin2hex(encoded_params) <<
        "\",\"gas\":\"" << GAS_LIMIT << "\"},\"latest\"" << "],\"id\":" << call_id << "}";
  } else {
    data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"0x" <<
        params << "\"],\"id\":" << call_id << "}";
  }

  string_data = data.str();
  LOG(INFO) << "\n======= REQUEST =======\n" << fname << ", " << params << '\n' <<
      string_data << "\n=====================";

  if (curl) {
    /* set the error buffer as empty before performing a request */
    curl_err_buf[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, string_data.c_str());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      size_t len = strlen(curl_err_buf);
      LOG(ERROR) << "Curl result code != CURLE_OK. Result code: " << res;
      if (len) {
        return status::internal(std::string(curl_err_buf, len));
      }
      return status::internal("CURL error");
    } else {
      return handle_message(fname);
    }
  } else {
    LOG(ERROR) << "No curl!";
    return status::internal("No curl!");
  }
}

status eth_contract::handle_message(const std::string& fname) {
  json j;
  uint32_t result_call_id;
  std::stringstream ss(message);
  try {
    ss >> j;
  } catch (const std::exception& e) {
    LOG(ERROR) << "Invalid JSON!\n" << e.what();
    return status::internal(e.what());
  }
  message = "";

  if (j.find("id") != j.end() && j["id"].is_number()) {
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
    if (obj.is_string()) {
      std::string error = obj["message"].get<std::string>();
      return status::internal(error);
    }
    return status::internal(obj.dump());
  } else if (j.find("result") != j.end()) {
    if (j["result"].is_string()) {
      std::string result = j["result"].get<std::string>();
      auto p_it = function_outputs.find(fname);
      if (p_it == function_outputs.end()) {
        return status::invalid_argument("Function signature is not found in function_outputs!");
      }
      if (p_it->second != "[]") {
        std::string bin = hex2bin(result.substr(2));
        std::string decoded = decode(p_it->second, bin);
        return status::ok(decoded);
      }
      return status::ok(result);
    }
    return status::ok(j["result"].dump());
  }
  return status::internal("No result and no error!?");
}

size_t eth_contract::curl_callback(void *contents, size_t size, size_t nmemb, std::string *s) {
  size_t new_length = size * nmemb;
  try {
    s->append(reinterpret_cast<char*>(contents), new_length);
    LOG(INFO) << "\n=== CHUNK ===\n"
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
