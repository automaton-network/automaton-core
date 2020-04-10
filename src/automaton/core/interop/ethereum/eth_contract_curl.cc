#include "automaton/core/interop/ethereum/eth_contract_curl.h"

#include <iomanip>
#include <string>
#include <utility>

#include "automaton/core/interop/ethereum/eth_helper_functions.h"
#include "automaton/core/io/io.h"

using automaton::core::io::bin2hex;
using automaton::core::common::status;

using json = nlohmann::json;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

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
    LOG(WARNING) << "No contract with address " << address;
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
      if (jobj.find("stateMutability") != jobj.end()) {
        std::string stateMutability = jobj["stateMutability"].get<std::string>();
        is_transaction = (stateMutability == "pure" || stateMutability == "view") ? false : true;
      } else {
        LOG(FATAL) << "Function doesn't have \"stateMutability\" !";
      }
      inputs << '[';
      if (jobj.find("inputs") != jobj.end()) {
        std::vector<json> v_inputs = jobj["inputs"].get<std::vector<json> >();
        for (size_t i = 0; i < v_inputs.size(); ++i) {
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
        for (size_t i = 0; i < v_outputs.size(); ++i) {
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

eth_contract::eth_contract(const std::string& url, const std::string& contract_address,
    const std::string& abi_json_string):call_ids(0), server(url), address(contract_address) {
  json j;
  std::stringstream ss(abi_json_string);
  try {
    ss >> j;
  } catch (const std::exception& e) {
    LOG(FATAL) << "Invalid json! " << e.what();
  }
  abi = j;
  parse_abi(abi);
}

eth_contract::~eth_contract() {}

status eth_contract::call(const std::string& fname, const std::string& params,
    const std::string& private_key, const std::string& value,
    const std::string& gas_price_, const std::string& gas_limit_) {
  uint32_t call_id = get_next_call_id();
  auto it = signatures.find(fname);
  if (it == signatures.end()) {
    return status::invalid_argument("Function signature is not found!");
  }

  auto p_it = function_inputs.find(fname);
  if (p_it == function_inputs.end()) {
    return status::invalid_argument("Function signature is not found in function_inputs!");
  }
  std::string encoded_params = "";
  if (p_it->second != "[]") {
    encoded_params = encode(p_it->second, params);
  }

  bool is_transaction = it->second.second;
  std::stringstream data;
  std::string string_data;

  if (!is_transaction) {
    data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":[{ \"to\":\"" << address <<
        "\",\"data\":\"" << it->second.first << bin2hex(encoded_params) <<
        "\",\"gas\":\"0x" << (gas_limit_ != "" ? gas_limit_ : gas_limit) <<
        "\"},\"latest\"" << "],\"id\":" << call_id << "}";
  } else {
    if (private_key != "") {
      std::string acc_address = get_address_from_prkey(private_key);
      if (acc_address.size() != 42) {
        return status::internal("Invalid private key! Could not generate address!");
      }

      data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendTransaction\",\"params\":[{ \"to\":\"" << address <<
          "\",\"from\":\"" << acc_address << "\",\"value\":\"" << value <<
          "\",\"data\":\"" << it->second.first << bin2hex(encoded_params) <<
          "\",\"gas\":\"0x" << (gas_limit_ != "" ? gas_limit_ : gas_limit) <<
          "\"}],\"id\":" << call_id << "}";
    } else {  // Raw transaction is given in params [for compatibility]
      data << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"0x" <<
          params << "\"],\"id\":" << call_id << "}";
    }
  }

  string_data = data.str();
  LOG(INFO) << "\n======= REQUEST =======\n" << fname << ", " << params << '\n' <<
      string_data << "\n=====================";

  status s = curl_post(server, string_data, call_id);
  if (s.code == status::OK) {
    if (!is_transaction) {
      return decode_function_result(fname, s.msg);
    } else {
      // TODO(kari): Decode transaction receipt
      return eth_getTransactionReceipt(server, s.msg);
    }
  }
  return s;
}

void eth_contract::set_gas_price(const std::string& new_gas_price_hex) {
  gas_price = new_gas_price_hex;
}

void eth_contract::set_gas_limit(const std::string& new_gas_limit_hex) {
  gas_limit = new_gas_limit_hex;
}

status eth_contract::decode_function_result(const std::string& fname, const std::string& result) {
  auto it = signatures.find(fname);
  if (it == signatures.end()) {
    return status::invalid_argument("Function signature is not found!");
  }
  auto p_it = function_outputs.find(fname);
  if (p_it == function_outputs.end()) {
    return status::invalid_argument("Function signature is not found in function_outputs!");
  }

  if (p_it->second != "[]") {
    std::string bin;
    if (result.find("0x") == 0) {
      bin = hex2bin(result.substr(2));
    } else {
      bin = hex2bin(result);
    }
    std::string decoded = decode(p_it->second, bin);
    return status::ok(decoded);
  }
  return status::ok(result);
}

uint32_t eth_contract::get_next_call_id() {
  std::lock_guard<std::mutex> lock(call_ids_mutex);
  return ++call_ids;
}

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton
