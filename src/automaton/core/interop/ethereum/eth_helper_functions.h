#ifndef AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_
#define AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_

#include <string>
#include <sstream>

#include <curl/curl.h>  // NOLINT
#include <json.hpp>

#include "automaton/core/common/status.h"
#include "automaton/core/io/io.h"

using automaton::core::common::status;

using json = nlohmann::json;

namespace automaton {
namespace core {
namespace interop {
namespace ethereum {

static size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s) {
  size_t new_length = size * nmemb;
  try {
    s->append(reinterpret_cast<char*>(contents), new_length);
  }
  catch (std::bad_alloc &e) {
    LOG(ERROR) << "Bad_alloc while reading data!";
    return 0;
  }
  return new_length;
}

static status handle_result(const std::string& result) {
  json j;
  std::stringstream ss(result);
  ss >> j;

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
      return status::ok(result.substr(2));
    }
    return status::ok(j["result"].dump());
  }
  return status::internal("No result and no error!?");
}

static status curl_post(const std::string& url, const std::string& data) {
  CURL *curl;
  CURLcode res;
  std::string message;
  char curl_err_buf[1024];

  curl = curl_easy_init();

  LOG(INFO) << "\n======= REQUEST =======\n" << data << "\n=====================";

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &message);
    curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");

    curl_err_buf[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      size_t len = strlen(curl_err_buf);
      LOG(ERROR) << "Curl result code != CURLE_OK. Result code: " << res;
      if (len) {
        return status::internal(std::string(curl_err_buf, len));
      }
      return status::internal("CURL error");
    } else {
      LOG(INFO) << "\n======= RESPONSE =======\n" << message << "\n=====================";
      return handle_result(message);
    }
  } else {
    return status::internal("No curl!");
  }
}

static status eth_getTransactionCount(const std::string& url, const std::string& address) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionCount\",\"params\":[\"" << address <<
      "\",\"latest\"" << "],\"id\":1}";
  return curl_post(url, ss.str());
}

static status eth_getCode(const std::string& url, const std::string& address) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getCode\",\"params\":[\"" << address <<
      "\",\"latest\"" << "],\"id\":1}";
  return curl_post(url, ss.str());
}

static status eth_gasPrice(const std::string& url) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_gasPrice\",\"params\":[],\"id\":1}";
  return curl_post(url, ss.str());
}

static status eth_getTransactionReceipt(const std::string& url, const std::string& tx_hash) {
  std::stringstream ss;
  ss << "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionReceipt\",\"params\":[\"" << tx_hash << "\"],\"id\":1}";
  return curl_post(url, ss.str());
}



}  // namespace ethereum
}  // namespace interop
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_INTEROP_ETHEREUM_ETH_HELPER_FUNCTIONS_H_
