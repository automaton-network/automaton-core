#include <fstream>
#include <string>
#include <vector>

#include <json.hpp>

#include "automaton/core/interop/ethereum/eth_contract_curl.h"
#include "automaton/core/interop/ethereum/eth_helper_functions.h"
#include "automaton/core/io/io.h"

#include "gtest/gtest.h"

using json = nlohmann::json;

using namespace automaton::core::interop::ethereum;  // NOLINT

using automaton::core::io::bin2hex;
using automaton::core::io::hex2bin;
using automaton::core::io::hex2dec;

TEST(encoder, encode_decode_test) {
  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  auto l_handler = logworker->addDefaultLogger("demo", "./");
  g3::initializeLogging(logworker.get());

  auto t = get_type("int");
  EXPECT_EQ(t.str, "int");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, no_array);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("int[][]");
  EXPECT_EQ(t.str, "int[][]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("fixed[5]");
  EXPECT_EQ(t.str, "fixed[5]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, fixed);
  EXPECT_EQ(t.array_len, 5);
  t = get_type("bytes32");
  EXPECT_EQ(t.str, "bytes32");
  EXPECT_EQ(t.s_type, fixed_size_bytes);
  EXPECT_EQ(t.s_array_type, no_array);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("string");
  EXPECT_EQ(t.str, "string");
  EXPECT_EQ(t.s_type, string);
  EXPECT_EQ(t.s_array_type, no_array);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("string[5][]");
  EXPECT_EQ(t.str, "string[5][]");
  EXPECT_EQ(t.s_type, string);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("string[][5]");
  EXPECT_EQ(t.str, "string[][5]");
  EXPECT_EQ(t.s_type, string);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 5);
  t = get_type("string[4][5]");
  EXPECT_EQ(t.str, "string[4][5]");
  EXPECT_EQ(t.s_type, string);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 5);
  t = get_type("int[4][5]");
  EXPECT_EQ(t.str, "int[4][5]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, fixed);
  EXPECT_EQ(t.array_len, 5);

  t = extract_array_type("uint256[][]");
  EXPECT_EQ(t.str, "uint256[]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 0);

  t = extract_array_type("string[5][]");
  EXPECT_EQ(t.str, "string[5]");
  EXPECT_EQ(t.s_type, string);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 5);

  t = extract_array_type("int[][5]");
  EXPECT_EQ(t.str, "int[]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, dynamic);
  EXPECT_EQ(t.array_len, 0);

  t = extract_array_type("int[5][]");
  EXPECT_EQ(t.str, "int[5]");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.s_array_type, fixed);
  EXPECT_EQ(t.array_len, 5);

  t = extract_array_type("bytes[]");
  EXPECT_EQ(t.str, "bytes");
  EXPECT_EQ(t.s_type, dynamic_size_bytes);
  EXPECT_EQ(t.s_array_type, no_array);
  EXPECT_EQ(t.array_len, 0);

  EXPECT_EQ(calculate_offset({"uint256"}), 32);
  EXPECT_EQ(calculate_offset({"uint8", "string", "string[][5]", "bool"}), 4 * 32);
  EXPECT_EQ(calculate_offset({"fixed[2][3][4]"}), 24 * 32);
  EXPECT_EQ(calculate_offset({"string", "bytes"}), 64);
  EXPECT_EQ(calculate_offset({"string[]", "uint32"}), 64);
  EXPECT_EQ(calculate_offset({"bool[3]", "uint256[7][]"}), 4 * 32);
  EXPECT_EQ(calculate_offset({"uint256[][10]"}), 32);
  EXPECT_EQ(calculate_offset({"uint256[10]"}), 320);

  std::string encoded, decoded, signatures, parameters;

  signatures = "[\"bytes\"]";
  parameters = "[\"A30B12\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint\"]";
  parameters = "[2]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, "[\"2\"]");

  signatures = "[\"uint[][]\",\"string[]\"]";
  parameters = "[[[1,2],[3]],[\"one\",\"two\",\"three\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, "[[[\"1\",\"2\"],[\"3\"]],[\"one\",\"two\",\"three\"]]");

  signatures = "[\"uint[]\",\"string\"]";
  parameters = "[[\"1\",\"2\"],\"aaa\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint\",\"string[2]\"]";
  parameters = "[\"15\",[\"aaa\",\"eee\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[2]\"]";
  parameters = "[[\"aaa\",\"eee\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2]\"]";
  parameters = "[[[\"aaa\",\"eee\"],[\"ppp\",\"ooo\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2]\",\"uint8[][]\"]";
  parameters = "[[[\"aaa\",\"eee\"],[\"ooo\",\"ppp\"]],[[\"7\",\"8\"],[\"5\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[2][2]\"]";
  parameters = "[[[\"aaa\",\"eee\"],[\"ooo\",\"ppp\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, "[[[\"aaa\",\"eee\"],[\"ooo\",\"ppp\"]]]");

  signatures = "[\"string[][2][]\"]";
  parameters = "[[[[\"aaa\"],[\"eee\"]],[[\"ooo\"],[\"ppp\",\"ooo\"]]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[2]\"]";
  parameters = "[[1,2]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, "[[\"1\",\"2\"]]");

  signatures = "[\"uint256[3][2]\"]";
  parameters = "[[[\"1\",\"2\",\"3\"],[5,6,7]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, "[[[\"1\",\"2\",\"3\"],[\"5\",\"6\",\"7\"]]]");

  signatures = "[\"uint256[3][2]\", \"string[][2][]\"]";
  parameters = "[[[\"1\",\"2\",\"3\"],[\"5\",\"6\",\"7\"]],[[[\"aaa\"],[\"eee\"]],[[\"ooo\"],[\"ppp\",\"ooo\"]]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2][]\", \"uint256[3][2]\"]";
  parameters = "[[[[\"aaa\"],[\"eee\"]],[[\"ooo\"],[\"ppp\",\"rrr\"]]],[[\"1\",\"2\",\"3\"],[\"5\",\"6\",\"7\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[3][2]\", \"string[2][2][]\"]";
  parameters = "[[[\"1\",\"2\",\"3\"],[\"5\",\"6\",\"7\"]],[[[\"aaa\",\"eee\"],[\"ppp\",\"rrr\"]]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"address\"]";
  parameters = "[\"22D9D6FAB361FAA969D2EFDE420472633CBB7B11\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"bool\"]";
  parameters = "[true]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  // Test contract functions

  const char* JSON_FILE = "../contracts/test_contract/build/contracts/test_contract.json";
  const char* URL = "127.0.0.1:7545";
  const char* CONTRACT_ADDR = "0xCfEB869F69431e42cdB54A4F4f105C19C080A601";
  const char* ADDRESS = "0x90F8bf6A479f320ead074411a4B0e7944Ea8c9C1";

  curl_global_init(CURL_GLOBAL_ALL);

  using namespace automaton::core::interop::ethereum;  // NOLINT

  std::fstream fs(JSON_FILE, std::fstream::in);
  json j, abi;
  if (!fs.is_open()) {
    LOG(FATAL) << "No contract json file!";
  }
  fs >> j;
  if (j.find("abi") != j.end()) {
    abi = j["abi"].get<json>();
  } else {
    LOG(FATAL) << "No abi!";
  }
  std::string abi_string = abi.dump();
  eth_contract::register_contract(URL, CONTRACT_ADDR, abi_string);
  fs.close();

  auto contract = eth_contract::get_contract(CONTRACT_ADDR);
  EXPECT_NE(contract, nullptr);

  status s = status::ok();

  s = contract->call("f1", "[3]");
  EXPECT_EQ(s.msg, "[\"259\"]");

  s = contract->call("f2", "[3]");
  EXPECT_EQ(s.msg, "[[\"0\",\"1\",\"4\"]]");

  s = contract->call("f3", "[5,7]");
  EXPECT_EQ(s.msg, "[\"7\",\"5\"]");

  s = contract->call("f4", "[\"abc\"]");
  EXPECT_EQ(s.msg, "[\"cba\"]");

  s = contract->call("f5", "");
  EXPECT_EQ(s.msg, "[\"ABCDEABCDE\"]");

  s = contract->call("f6", "");
  EXPECT_EQ(s.msg, "[\"CFEB869F69431E42CDB54A4F4F105C19C080A601\"]");

  s = contract->call("f7", "[true]");
  EXPECT_EQ(s.msg, "[false]");
  s = contract->call("f7", "[false]");
  EXPECT_EQ(s.msg, "[true]");

  s = contract->call("f8", "[5]");
  EXPECT_EQ(s.msg, "[\"-5\"]");
  s = contract->call("f8", "[0]");
  EXPECT_EQ(s.msg, "[\"0\"]");
  s = contract->call("f8", "[\"18446744073709551615\"]");
  EXPECT_EQ(s.msg, "[\"-18446744073709551615\"]");
  s = contract->call("f8", "[\"518446744073709551615\"]");
  EXPECT_EQ(s.msg, "[\"-518446744073709551615\"]");

  s = contract->call("f9", "[5]");
  EXPECT_EQ(s.msg, "[\"10\"]");

  s = contract->call("f10", "[[[2,3],[5,7],[11,13]]]");
  EXPECT_EQ(s.msg, "[\"30030\"]");

  s = contract->call("f11", "[[[2,3,5],[7,11,13]]]");
  EXPECT_EQ(s.msg, "[\"30030\"]");

  s = contract->call("f12", "[[1,2,3,4,5]]");
  EXPECT_EQ(s.msg, "[\"15\"]");

  s = contract->call("f13", "");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");

  s = contract->call("f14", "");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");

  s = contract->call("f15", "");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");

  s = contract->call("f16", "");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");

  s = contract->call("f17", "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\"],[\"1\",\"2\"],[\"2\",\"3\"]]]");

  s = contract->call("f18", "[[[\"0\",\"1\"],[\"1\",\"2\"],[\"2\",\"3\"]]]");
  EXPECT_EQ(s.msg, "[[[\"0\",\"1\",\"2\"],[\"1\",\"2\",\"3\"]]]");

  s = contract->call("f19", "[[[\"aa\",\"bb\",\"cc\"],[\"ss\",\"pp\",\"rr\"]]]");
  EXPECT_EQ(s.msg, "[[[\"aa\",\"ss\"],[\"bb\",\"pp\"],[\"cc\",\"rr\"]]]");

  s = contract->call("f20", "[[[\"aa\",\"ss\"],[\"bb\",\"pp\"],[\"cc\",\"rr\"]]]");
  EXPECT_EQ(s.msg, "[[[\"aa\",\"bb\",\"cc\"],[\"ss\",\"pp\",\"rr\"]]]");

  s = contract->call("f21",
  "[[[\"0\",\"1\",\"2\",\"3\"],[\"4\",\"5\",\"6\",\"7\"]],[[[\"a\",\"ab\"],[\"abc\"]],[[\"b\",\"ba\"],[\"cb\",\"ca\",\"c\"]]]]]");  // NOLINT
  EXPECT_EQ(s.msg, "[[[[\"4\",\"5\",\"6\",\"7\"]],[[\"0\",\"1\",\"2\",\"3\"]]],true,[[\"a\",\"ab\"],[\"abc\"],[\"b\",\"ba\"],[\"cb\",\"ca\",\"c\"]]]");  // NOLINT

  s = contract->call("f22", "[[[[\"aa\"],[\"ss\"]],[[\"bb\"],[\"pp\"]],[[\"cc\"],[\"rr\"]]]]");
  EXPECT_EQ(s.msg, "[[[[\"aa\",\"bb\",\"cc\"],[\"ss\",\"pp\",\"rr\"]]]]");
}
