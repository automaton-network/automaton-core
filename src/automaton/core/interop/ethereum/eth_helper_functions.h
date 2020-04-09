#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_

#include <cryptopp/integer.h>
#include <curl/curl.h>

#include <cmath>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <json.hpp>
#include <secp256k1.h>  // NOLINT
#include <secp256k1_recovery.h>  // NOLINT

#include "gtest/gtest.h"

#include "automaton/core/common/status.h"
#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/io/io.h"
#include "automaton/tools/miner/miner.h"

using json = nlohmann::json;

using automaton::core::common::status;
using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::io::bin2hex;
using automaton::core::io::dec2hex;
using automaton::core::io::hex2bin;
using automaton::core::io::hex2dec;
using automaton::tools::miner::sign;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

static const uint32_t ERROR_BUF_SIZE = 1024;

struct curl_struct {
  CURL* curl;
  curl_slist* list;
  char curl_err_buf[ERROR_BUF_SIZE];
  std::string msg_buf;
};

/**
 Returns Keccak_256 hash of data as 32-byte string.
*/
inline std::string hash(const std::string& data) {
  Keccak_256_cryptopp hasher;
  uint8_t digest[32];
  hasher.calculate_digest(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  return std::string(reinterpret_cast<char*>(digest), 32);
}

inline size_t curl_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
  size_t new_length = size* nmemb;
  try {
    s->append(reinterpret_cast<char*>(contents), new_length);
    LOG(INFO) << "\n=== CHUNK ===\n"
        << std::string(reinterpret_cast<char*>(contents), new_length) << "\n ===== EoCH ====";
  }
  catch (std::bad_alloc& e) {
    LOG(WARNING) << "Bad_alloc while reading data! " << e.what();
    return 0;
  }
  return new_length;
}

inline curl_struct* create_new_curl_struct(const std::string& url) {
  CURL* c = curl_easy_init();
  if (c) {
    curl_struct* c_struct = new curl_struct();
    curl_slist* list = nullptr;
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, c_struct->curl_err_buf);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &(c_struct->msg_buf));
    curl_easy_setopt(c, CURLOPT_ENCODING, "gzip");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, list);

    c_struct->curl = c;
    c_struct->list = list;
    return c_struct;
  }
  return nullptr;
}

inline void destroy_curl_struct(curl_struct* c_struct) {
  if (!c_struct) {
    return;
  }
  curl_slist_free_all(c_struct->list);
  curl_easy_cleanup(c_struct->curl);
  delete c_struct;
}

inline status handle_result(const std::string& message, uint32_t call_id) {
  json j;
  uint32_t result_call_id;
  std::stringstream ss(message);
  try {
    ss >> j;
  } catch (const std::exception& e) {
    std::stringstream ss_error;
    ss_error << "Invalid JSON! " << e.what();
    LOG(WARNING) << ss_error.str();
    return status::internal(ss_error.str());
  }

  if (j.find("id") != j.end() && j["id"].is_number()) {
    result_call_id = j["id"].get<uint32_t>();
  } else {
    LOG(WARNING) << "ID not found!";
    return status::internal("ID not found!");
  }

  if (result_call_id != call_id) {
    std::stringstream ss_error;
    ss_error << "Result ID " << result_call_id << " does not match request ID: " << call_id;
    LOG(WARNING) << ss_error.str();
    return status::internal(ss_error.str());
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
      return status::ok(j["result"].get<std::string>());
    }
    return status::ok(j["result"].dump());
  }
  return status::internal("No result and no error!? Message: \n" + message);
}

inline status curl_post(const std::string& url, const std::string& data, uint32_t call_id) {
  LOG(INFO) << "\n======= REQUEST =======\n" << data << "\n=====================";
  curl_struct* c_struct = create_new_curl_struct(url);
  status s = status::ok();
  if (c_struct) {
    /* set the error buffer as empty before performing a request */
    c_struct->curl_err_buf[0] = '\0';
    curl_easy_setopt(c_struct->curl, CURLOPT_POSTFIELDS, data.c_str());
    CURLcode c_code = curl_easy_perform(c_struct->curl);
    if (c_code != CURLE_OK) {
      size_t len = strlen(c_struct->curl_err_buf);
      LOG(WARNING) << "Curl result code != CURLE_OK. Result code: " << c_code;
      if (len) {
        s = status::internal(std::string(c_struct->curl_err_buf, len));
      } else {
        s = status::internal("CURL error");
      }
    } else {
      LOG(INFO) << "\n======= RESPONSE =======\n" << c_struct->msg_buf << "\n=====================";
      s = handle_result(c_struct->msg_buf, call_id);
    }
  } else {
    LOG(WARNING) << "No curl!";
    s = status::internal("No curl!");
  }

  destroy_curl_struct(c_struct);
  return s;
}

inline status eth_getTransactionCount(const std::string& url, const std::string& address) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionCount\",\"params\":[\"" << address <<
      "\",\"latest\"" << "],\"id\":1}";
  return curl_post(url, ss.str(), 1);
}

inline status eth_getCode(const std::string& url, const std::string& address) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getCode\",\"params\":[\"" << address <<
      "\",\"latest\"" << "],\"id\":2}";
  return curl_post(url, ss.str(), 2);
}

inline status eth_getBalance(const std::string& url, const std::string& address) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBalance\",\"params\":[\"" << address <<
      "\",\"latest\"" << "],\"id\":3}";
  return curl_post(url, ss.str(), 3);
}

inline status eth_gasPrice(const std::string& url) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_gasPrice\",\"params\":[],\"id\":4}";
  return curl_post(url, ss.str(), 4);
}

inline status eth_getTransactionReceipt(const std::string& url, const std::string& tx_hash) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionReceipt\",\"params\":[\"" << tx_hash << "\"],\"id\":5}";
  return curl_post(url, ss.str(), 5);
}

inline std::string get_address_from_prkey(const std::string& private_key_hex) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  std::string pr_key_bin = hex2bin(private_key_hex);

  if (!secp256k1_ec_pubkey_create(context, pubkey, reinterpret_cast<const uint8_t*>(pr_key_bin.data()))) {
    LOG(WARNING) << "Invalid private key!";
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }

  unsigned char pub_key_serialized[65];
  size_t outLen = 65;
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &outLen, pubkey, SECP256K1_EC_UNCOMPRESSED);
  delete pubkey;
  secp256k1_context_destroy(context);

  std::string pub_key(reinterpret_cast<char*>(&pub_key_serialized[1]), 64);
  std::string pub_key_hash = hash(pub_key);
  return "0x" + bin2hex(pub_key_hash.substr(12));
}

/**
 Signs a message using secp256k1 also checks if a public key can be created from the given private key. If the check
 fails, empty string will be returned. Returns concatenated r, s and v values of the signature.
 @param[in] priv_key byte array repesenting the private key
 @param[in] message_hash 32-byte string.
*/
inline std::string sign_and_verify(const unsigned char* priv_key, const unsigned char* message_hash) {
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();

  std::string rsv = sign(priv_key, message_hash);

  if (!secp256k1_ec_pubkey_create(context, pubkey, priv_key)) {
    LOG(WARNING) << "Invalid private key!!!" << bin2hex(std::string(reinterpret_cast<const char*>(priv_key), 32));
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }
  delete pubkey;
  secp256k1_context_destroy(context);
  return rsv;
}

/**
 Returns recovered Ethereum address from a message and a signature.
*/
inline std::string recover_address(const unsigned char* rsv, const unsigned char* message_hash) {
  int32_t v = rsv[64];
  v -= 27;
  secp256k1_context* context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature signature;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(context, &signature, (unsigned char*)rsv, v)) {
    LOG(WARNING) << "Cannot parse signature!";
    secp256k1_context_destroy(context);
    return "";
  }
  secp256k1_pubkey* pubkey = new secp256k1_pubkey();
  if (!secp256k1_ecdsa_recover(context, pubkey, &signature, (unsigned char*) message_hash)) {
    LOG(WARNING) << "Cannot recover signature!";
    delete pubkey;
    secp256k1_context_destroy(context);
    return "";
  }

  size_t out_len = 65;
  unsigned char pub_key_serialized[65];
  secp256k1_ec_pubkey_serialize(context, pub_key_serialized, &out_len, pubkey, SECP256K1_EC_UNCOMPRESSED);
  std::string pub_key_uncompressed(reinterpret_cast<char*>(pub_key_serialized), out_len);
  delete pubkey;
  secp256k1_context_destroy(context);
  return hash(pub_key_uncompressed.substr(1)).substr(12);
}

// Argument encoding and decoding

/* TODO(kari):
  	* Replace type bitmask with variables
  	* Type structure is messy and confusing - clean it up
  	* Use switch after replacing the bitmask
  	* Move convertion functions to the io module
    * Add support for fixed-point numbers
*/

static const uint32_t BUFFER_SIZE = 2048;

enum abi_type {
  empty = 0,
  numerical = 1,  // bool, int<>, uint<>, fixed, ufixed, fixed<M>x<N>, ufixed<M>x<N>
  fixed_size_bytes = 2,  // bytes<>, function(bytes24), address(uint160)
  dynamic_size_bytes = 3,  // bytes
  string = 4,  // string
};

enum abi_array_type {
  no_array = 0,
  fixed = 1,
  dynamic = 2
};

inline std::ostream& operator<<(std::ostream& os, const abi_type& obj) {
  switch (obj) {
    case numerical: os << "numerical"; break;
    case string: os << "string"; break;
    case fixed_size_bytes: os << "fixed_size_bytes"; break;
    case dynamic_size_bytes: os << "dynamic_size_bytes"; break;
    default: os << "no type"; break;
  }
  return os;
}

struct type {
  std::string str = "";
  abi_type s_type = empty;
  abi_array_type s_array_type = no_array;
  uint32_t array_len = 0;
};

inline std::ostream& operator<<(std::ostream& os, const type& obj) {
  os << "TYPE{" << obj.str << ", " << obj.s_type;
  if (obj.s_array_type != no_array) {
    if (obj.s_array_type == fixed) {
      os << ", fixed array of size ";
    } else {
      os << ", dynamic array of size ";
    }
    os << obj.array_len;
  }
  os << "}";
  return os;
}

inline type get_type(std::string s) {
  type t;
  t.str = s;
  t.s_array_type = no_array;
  t.array_len = 0;

  bool is_array = false;
  // Check if it is an array and its size if any
  size_t k1 = 0, k2 = 0, pos;
  pos = s.find('[');
  while (pos != std::string::npos) {
    k1 = pos;
    pos = s.find('[', pos + 1);
  }
  if (k1 != 0) {
    k2 = s.find(']', k1 + 1);
  }
  if (k1 && k2 != std::string::npos) {
    is_array = true;
    if (k1 < (k2 - 1)) {
      std::string len_str = s.substr(k1 + 1, k2 - k1 - 1);
      t.array_len = stoi(len_str);
    }
  }

  if (is_array) {
    type arr_type = get_type(s.substr(0, k1));
    t.s_type = arr_type.s_type;
    if (arr_type.s_array_type == dynamic || t.array_len == 0 ||
        t.s_type == dynamic_size_bytes || t.s_type == string) {
      t.s_array_type = dynamic;
    } else {
      t.s_array_type = fixed;
    }
  } else {
    if (s == "function" || (s.find("bytes") == 0 && s.size() > 5) || s == "address") {
      t.s_type = fixed_size_bytes;
    } else if (s == "bytes") {
      t.s_type = dynamic_size_bytes;
    } else if (s == "string") {
      t.s_type = string;
    } else {
      t.s_type = numerical;  // TODO(kari): Could be invalid type
    }
  }
  return t;
}

inline type extract_array_type(std::string s) {
  type new_t;
  size_t k1 = 0, k2 = 0, pos = 0;
  pos = s.find('[', pos + 1);
  while (pos != std::string::npos) {
    k1 = pos;
    pos = s.find('[', pos + 1);
  }
  if (k1 != 0) {
    k2 = s.find(']', k1 + 1);
  }
  if (k1 && k2 != std::string::npos) {
    s.erase(k1, k2 - k1 + 1);
    new_t = get_type(s);
  } else {
    LOG(WARNING) << "Array expected!: " << s;
  }
  return new_t;
}

inline uint32_t calculate_offset(std::vector<std::string> params) {
  uint32_t tail_offset = 0;
  for (uint32_t i = 0; i < params.size(); ++i) {
    type t = get_type(params[i]);
    if (t.s_array_type == fixed) {
      uint32_t element_count = 1;
      // Add the other dimenstions if any
      while (t.s_array_type == fixed) {
        element_count *= t.array_len;
        t = extract_array_type(t.str);
      }
      tail_offset += 32 * element_count;
    } else {
      tail_offset += 32;
    }
  }
  return tail_offset;
}

inline std::string encode_string(const std::string& byte_array) {
  std::stringstream ss;
  auto reminder = byte_array.size() % 32;
  ss << byte_array << (reminder ? std::string(32 - reminder, '\0') : "");
  return ss.str();
}

inline std::string dec_to_32hex(uint32_t n) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(64) << std::hex << n;
  return ss.str();
}

inline std::string u64_to_u256(uint64_t n) {
  char bytes[8];
  bytes[7] = (n) & 0xFF;
  bytes[6] = (n >> 8) & 0xFF;
  bytes[5] = (n >> 16) & 0xFF;
  bytes[4] = (n >> 24) & 0xFF;
  bytes[3] = (n >> 32) & 0xFF;
  bytes[2] = (n >> 40) & 0xFF;
  bytes[1] = (n >> 48) & 0xFF;
  bytes[0] = (n >> 56) & 0xFF;

  return std::string(24, '\0') + std::string(bytes, 8);
}

inline std::string dec_to_i256(bool is_signed, std::string s) {
  CryptoPP::Integer::Signedness sign = is_signed ? CryptoPP::Integer::SIGNED : CryptoPP::Integer::UNSIGNED;
  if (s.substr(0, 2) == "0x") {
    s = "h" + s.substr(2);
  }
  CryptoPP::Integer i(s.c_str());
  uint8_t bytes[32] = {0};
  i.Encode(bytes, 32, sign);
  return std::string(reinterpret_cast<const char*>(bytes), 32);
}

inline std::string i256_to_dec(bool is_signed, const std::string& s) {
  // TODO(kari): If the number is more than 64 bits, throw error or use only 64 bits of it.
  if (s.size() == 0) {
    LOG(WARNING) << "Invalid argument for i256!";
    return "";
  }
  CryptoPP::Integer::Signedness sign = is_signed ? CryptoPP::Integer::SIGNED : CryptoPP::Integer::UNSIGNED;
  CryptoPP::Integer i(reinterpret_cast<const uint8_t*>(s.c_str()), 32, sign);
  std::stringstream ss;
  ss << i;
  std::string res = ss.str();
  return res.substr(0, res.size() - 1);  // remove suffix
}

inline uint64_t u256_to_u64(const std::string& s) {
  if (s.size() != 32) {
    LOG(WARNING) << "Invalid argument format!" << s;
    return 0;
  }
  uint64_t n = 0;
  for (auto i = 0; i < 8; i++) {
    uint64_t k = reinterpret_cast<const uint8_t &>(s[24 + i]);
    n |= (k << ((7 - i) * 8));
  }
  return n;
}

inline void check_and_resize_buffer(char** buffer, size_t* size, size_t pos, size_t data_size) {
  size_t new_size = *size;
  while (pos + data_size > new_size) {
    new_size += BUFFER_SIZE;
  }
  if (*size != new_size) {
    char* new_buffer = new char[new_size];
    memcpy(new_buffer, *buffer, *size);
    delete [] *buffer;
    *buffer = new_buffer;
    *size = new_size;
  }
}

/**
 Returns RLP encoding of s in hex.
 @param[in] s data to be encode, MUST be in hex WITHOUT '0x' prefix.
 @param[in] is_list shows if s represents one element or the payload of a list.
*/
inline std::string rlp_encode(std::string s, bool is_list) {
  if (s.size() % 2) {
    s = "0" + s;
  }
  uint32_t length = static_cast<uint32_t>(s.size()) / 2;
  if (length == 0) {
    return "80";
  }
  if (length == 1 && hex2dec(s.substr(0, 2)) < 128) {
    return s;
  }
  std::stringstream ss;
  if (length < 56) {
    ss << dec2hex((is_list ? 192 : 128) + length) << s;
  } else {
    std::string length_in_hex = dec2hex(length);
    uint32_t first_byte = (is_list ? 247 : 183) + (static_cast<uint32_t>(length_in_hex.size()) / 2);
    // todo: check if this byte is valid
    ss << dec2hex(first_byte) << length_in_hex << s;
  }
  return ss.str();
}

inline void encode_param(type t, const json& json_data, char** buffer, size_t* buf_size,
    size_t* head_pos, size_t* tail_pos) {
  if (t.s_array_type == fixed) {
    if (!json_data.is_array() || json_data.size() != t.array_len) {
      LOG(WARNING) << "Invalid argument! Expected array with length " << t.array_len << ", got " << json_data;
      return;
    }
    type tp = extract_array_type(t.str);
    for (auto it = json_data.begin(); it != json_data.end(); ++it) {
      encode_param(tp, *it, buffer, buf_size, head_pos, tail_pos);
    }
  } else if (t.s_array_type == dynamic) {
    if (!json_data.is_array()) {
      LOG(WARNING) << "Invalid argument! Expected array, got " << json_data;
      return;
    }
    if (t.array_len && json_data.size() != t.array_len) {
      LOG(WARNING) << "Invalid argument! Expected array with length " << t.array_len << ", got " << json_data;
      return;
    }
    std::string encoded;
    if (t.array_len == 0) {  // Size is unknown; not fixed
      encoded = u64_to_u256(json_data.size());
      check_and_resize_buffer(buffer, buf_size, *tail_pos, 32);
      memcpy(*buffer + *tail_pos, encoded.c_str(), 32);
      *tail_pos += 32;
    }
    size_t head_prim_pos = *tail_pos;
    size_t tail_prim_pos = *tail_pos;
    type tp = extract_array_type(t.str);
    if (tp.s_array_type == fixed) {
      tail_prim_pos += (json_data.size() * calculate_offset({tp.str}));
      for (auto it = json_data.begin(); it != json_data.end(); ++it) {
        encode_param(tp, *it, buffer, buf_size, &head_prim_pos, &tail_prim_pos);
      }
    } else if (tp.s_type == dynamic_size_bytes || tp.s_type == string || tp.s_array_type == dynamic) {
      tail_prim_pos += (32 * json_data.size());
      for (auto it = json_data.begin(); it != json_data.end(); ++it) {
        encoded = u64_to_u256(tail_prim_pos - *tail_pos);
        check_and_resize_buffer(buffer, buf_size, head_prim_pos, 32);
        memcpy(*buffer + head_prim_pos, encoded.c_str(), 32);
        head_prim_pos += 32;
        encode_param(tp, *it, buffer, buf_size, &tail_prim_pos, &tail_prim_pos);
      }
    } else if (tp.s_type == numerical || tp.s_type == fixed_size_bytes) {
      tail_prim_pos += (32 * json_data.size());
      for (auto it = json_data.begin(); it != json_data.end(); ++it) {
        encode_param(tp, *it, buffer, buf_size, &head_prim_pos, &tail_prim_pos);
      }
    } else {
      LOG(WARNING) << "Invalid type!" << tp.s_type;
      return;
    }
    *tail_pos = tail_prim_pos;
  } else if (t.s_type == string || t.s_type == dynamic_size_bytes) {
    if (!json_data.is_string()) {
      LOG(WARNING) << "Invalid argument!";
      return;
    }
    std::string s;
    std::string data = json_data.get<std::string>();
    if (t.s_type == dynamic_size_bytes) {
      s = hex2bin(data);
    } else {
      s = data;
    }
    std::string encoded = u64_to_u256(s.size());
    check_and_resize_buffer(buffer, buf_size, *tail_pos, 32);
    memcpy(*buffer + *tail_pos, encoded.c_str(), 32);
    *tail_pos += 32;
    encoded = encode_string(s);
    check_and_resize_buffer(buffer, buf_size, *tail_pos, encoded.size());
    memcpy(*buffer + *tail_pos, encoded.c_str(), encoded.size());
    *tail_pos += encoded.size();
  } else if (t.s_type == fixed_size_bytes) {
    if (!json_data.is_string()) {
      LOG(WARNING) << "Invalid argument! Expected string, got " << json_data;
      return;
    }
    std::string data = json_data.get<std::string>();
    std::string bin_data = hex2bin(data);
    if (bin_data.size() > 32) {
      LOG(WARNING) << "Invalid argument! Size > 32 bytes! " << data;
      return;
    }
    std::string encoded;
    if (t.str == "address") {
      if (bin_data.size() != 20) {
        LOG(WARNING) << "Invalid argument! Address size != 20 bytes! " << data;
        return;
      }
      encoded = std::string(12, '\0') + bin_data;
    } else {
      encoded = bin_data + std::string(32 - bin_data.size(), '\0');
    }
    check_and_resize_buffer(buffer, buf_size, *head_pos, 32);
    memcpy(*buffer + *head_pos, encoded.c_str(), 32);
    *head_pos += 32;
  } else if (t.s_type == numerical) {
    std::string encoded;
    if (t.str == "bool") {
      if (!json_data.is_boolean()) {
        LOG(WARNING) << "Invalid argument! Expected boolean, got " << json_data;
        return;
      }
      encoded = u64_to_u256(json_data.get<bool>() ? 1 : 0);
    } else if (t.str.find("int") != std::string::npos) {
      std::string s;
      if (json_data.is_number()) {
        s = std::to_string(json_data.get<uint64_t>());
      } else if (json_data.is_string()) {
        s = json_data.get<std::string>();
      } else {
        LOG(WARNING) << "Invalid argument! Expected number or string, got " << json_data;
        return;
      }
      bool is_signed = t.str[0] != 'u';
      encoded = dec_to_i256(is_signed, s);
    } else if (t.str.find("fixed") != std::string::npos) {
      LOG(WARNING) << "Unsuported data type double!";
      return;
    }
    check_and_resize_buffer(buffer, buf_size, *head_pos, 32);
    memcpy(*buffer + *head_pos, encoded.c_str(), 32);
    *head_pos += 32;
  } else {
    LOG(WARNING) << "Undefined type: " << t.s_type;
  }
}

inline std::string encode(const std::string& signatures_json, const std::string& parameters_json) {
  json j_sigs, j_params;
  try {
    std::stringstream sigs(signatures_json);
    sigs >> j_sigs;
    std::stringstream params(parameters_json);
    params >> j_params;
  } catch (const std::exception& e) {
    LOG(WARNING) << "Json error: " << e.what();
    return "";
  }

  if (!j_params.is_array() || j_params.size() != j_sigs.size()) {
    LOG(WARNING) << "Invalid arguments!";
    return "";
  }

  char* buffer = new char[BUFFER_SIZE];
  size_t buf_size = BUFFER_SIZE;

  std::vector<std::string> signatures;
  try {
    signatures = j_sigs.get<std::vector<std::string> >();
  } catch (const std::exception& e) {
    LOG(WARNING) << "Invalid arguments! " << e.what();
    return "";
  }

  size_t head_pos = 0;
  size_t tail_pos = calculate_offset(signatures);
  std::string signature;
  auto p_it = j_params.begin();
  uint32_t s_i = 0;
  for (; s_i < signatures.size() && p_it != j_params.end(); ++s_i, ++p_it) {
    signature = signatures[s_i];
    type t = get_type(signature);
    if (t.s_type == dynamic_size_bytes || t.s_type == string || t.s_array_type == dynamic) {
      std::string encoded = u64_to_u256(tail_pos);
      check_and_resize_buffer(&buffer, &buf_size, tail_pos, 32);
      memcpy(buffer + head_pos, encoded.c_str(), 32);
      head_pos += 32;
      encode_param(t, *p_it, &buffer, &buf_size, &tail_pos, &tail_pos);
    } else {
      encode_param(t, *p_it, &buffer, &buf_size, &head_pos, &tail_pos);
    }
  }
  std::string result = std::string(buffer, tail_pos);
  delete [] buffer;
  return result;
}

inline std::string decode_param(type t, const std::string& data, size_t pos) {
  if (t.s_array_type == fixed) {
    auto tp = extract_array_type(t.str);
    std::stringstream ss;
    ss << '[';
    if (tp.s_array_type == fixed) {
      for (uint32_t i = 1; i < t.array_len; ++i) {
        ss << decode_param(tp, data, pos) << ",";
        pos += calculate_offset({tp.str});
      }
      ss << decode_param(tp, data, pos);
    } else if (tp.s_type == numerical || tp.s_type == fixed_size_bytes) {
      for (uint32_t i = 1; i < t.array_len; ++i) {
        ss << decode_param(tp, data, pos) << ",";
        pos += 32;
      }
      ss << decode_param(tp, data, pos);
    }
    ss << ']';
    return ss.str();
  } else if (t.s_array_type == dynamic) {
    int64_t len = t.array_len;
    if (len == 0) {
      std::string s = data.substr(pos, 32);
      len = u256_to_u64(s);
      pos += 32;
      if (len == 0) {
        return "[]";
      }
    }
    std::stringstream ss;
    ss << '[';
    auto tp = extract_array_type(t.str);
    if (tp.s_array_type == no_array && (tp.s_type == numerical || tp.s_type == fixed_size_bytes)) {
      for (int32_t i = 1; i < len; ++i) {
        ss << decode_param(tp, data, pos) << ",";
        pos += 32;
      }
      ss << decode_param(tp, data, pos);
    } else if (tp.s_array_type == fixed) {
      for (int32_t i = 1; i < len; ++i) {
        ss << decode_param(tp, data, pos) << ",";
        pos += calculate_offset({tp.str});
      }
      ss << decode_param(tp, data, pos);
    } else if (tp.s_type == string || tp.s_type == dynamic_size_bytes || tp.s_array_type == dynamic) {
      for (int32_t i = 1; i < len; ++i) {
        std::string s = data.substr(pos + (32 * (i - 1)), 32);
        size_t offset = static_cast<size_t>(u256_to_u64(s));
        ss << decode_param(tp, data, pos + offset) << ",";
      }
      std::string s = data.substr(pos + static_cast<size_t>((32 * (len - 1))), 32);
      size_t offset = static_cast<size_t>(u256_to_u64(s));
      ss << decode_param(tp, data, pos + offset);
    } else {
      LOG(WARNING) << "Invalid type!";
      return "";
    }
    ss << ']';
    return ss.str();
  } else if (t.s_type == dynamic_size_bytes || t.s_type == string) {
    std::string s = data.substr(pos, 32);
    size_t len = static_cast<size_t>(u256_to_u64(s));
    pos += 32;
    std::string res = data.substr(pos, len);
    if (t.s_type == string) {
      return '"' + res + '"';
    }
    return '"' + bin2hex(res) + '"';
  } else if (t.s_type == numerical) {
    std::string res = data.substr(pos, 32);
    if (t.str == "bool") {
      size_t k = static_cast<size_t>(u256_to_u64(res));
      return k > 0 ? "true" : "false";
    } else if (t.str.find("int") != std::string::npos) {
      bool is_signed = t.str[0] != 'u';
      return '"' + i256_to_dec(is_signed, res) + '"';
    } else if (t.str.find("fixed") != std::string::npos) {
      LOG(WARNING) << "Unsuported data type double!";
      return "\"\"";
    }
    return "";
  } else if (t.s_type == fixed_size_bytes) {
    if (t.str == "address") {
      std::string res = data.substr(pos, 32);
      return '"' + bin2hex(res.substr(12, 20)) + '"';
    }
    std::regex rgx_sim("bytes(\\d+)");
    std::smatch match;
    const std::string sig = t.str;
    uint32_t k = 32;
    if (t.str == "function") {
      k = 24;
    } else if (std::regex_match(sig.begin(), sig.end(), match, rgx_sim) && match.size() == 2) {
      k = std::stoul(match[1]);
    }
    std::string res = data.substr(pos, k);
    return '"' + bin2hex(res) + '"';
  } else {
    LOG(WARNING) << "Undefined type: " << t.s_type;
  }
  return "";
}

inline std::string decode(const std::string& signatures_json, const std::string& data) {
  json j_sigs, j_params;
  try {
    std::stringstream sigs(signatures_json);
    sigs >> j_sigs;
  } catch (const std::exception& e) {
    LOG(WARNING) << "Json error: " << e.what();
    return "";
  }

  std::stringstream ss;
  std::string signature;
  ss << '[';
  size_t pos = 0;
  for (auto s_it = j_sigs.begin(); s_it != j_sigs.end();) {
    signature = (*s_it).get<std::string>();
    type t = get_type(signature);
    if (t.s_type == dynamic_size_bytes || t.s_type == string || t.s_array_type == dynamic) {
      std::string s = data.substr(pos, 32);
      size_t offset = static_cast<size_t>(u256_to_u64(s));
      ss << decode_param(t, data, offset);
    } else {
      ss << decode_param(t, data, pos);
    }
    if (++s_it != j_sigs.end()) {
      ss << ',';
    }
    if (t.s_array_type == fixed) {
      pos += calculate_offset({t.str});
    } else {
      pos += 32;
    }
  }
  ss << ']';
  return ss.str();
}

}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_
