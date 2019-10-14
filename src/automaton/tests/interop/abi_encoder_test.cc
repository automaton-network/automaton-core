#include <string>
#include <vector>

#include <json.hpp>

#include "automaton/core/interop/ethereum/eth_helper_functions.h"
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using json = nlohmann::json;

using automaton::core::interop::ethereum::calculate_offset;
using automaton::core::interop::ethereum::decode;
using automaton::core::interop::ethereum::encode;
using automaton::core::interop::ethereum::extract_array_type;
using automaton::core::interop::ethereum::get_type;
using automaton::core::interop::ethereum::numerical;
using automaton::core::interop::ethereum::fixed_size_bytes;
using automaton::core::interop::ethereum::fixed_size_array;
using automaton::core::interop::ethereum::dynamic_size_bytes;
using automaton::core::interop::ethereum::dynamic_size_array;
using automaton::core::io::bin2hex;
using automaton::core::io::hex2bin;
using automaton::core::io::hex2dec;

int main() {
  std::unique_ptr<g3::LogWorker> logworker {g3::LogWorker::createLogWorker()};
  g3::initializeLogging(logworker.get());

  auto t = get_type("int");
  EXPECT_EQ(t.str, "int");
  EXPECT_EQ(t.s_type, numerical);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("int[][]");
  EXPECT_EQ(t.str, "int[][]");
  EXPECT_EQ(t.s_type, numerical | dynamic_size_array);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("fixed[5]");
  EXPECT_EQ(t.str, "fixed[5]");
  EXPECT_EQ(t.s_type, numerical | fixed_size_array);
  EXPECT_EQ(t.array_len, 5);
  t = get_type("bytes32");
  EXPECT_EQ(t.str, "bytes32");
  EXPECT_EQ(t.s_type, fixed_size_bytes);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("string");
  EXPECT_EQ(t.str, "string");
  EXPECT_EQ(t.s_type, dynamic_size_bytes);
  EXPECT_EQ(t.array_len, 0);
  t = get_type("string[5][]");
  EXPECT_EQ(t.str, "string[5][]");
  EXPECT_EQ(t.s_type, dynamic_size_bytes | fixed_size_array);
  EXPECT_EQ(t.array_len, 5);

  t = extract_array_type("uint256[][]");
  EXPECT_EQ(t.str, "uint256[]");
  EXPECT_EQ(t.s_type, numerical | dynamic_size_array);
  EXPECT_EQ(t.array_len, 0);

  t = extract_array_type("string[5][]");
  EXPECT_EQ(t.str, "string[]");
  EXPECT_EQ(t.s_type, dynamic_size_bytes | dynamic_size_array);
  EXPECT_EQ(t.array_len, 0);

  t = extract_array_type("int[][5]");
  EXPECT_EQ(t.str, "int[5]");
  EXPECT_EQ(t.s_type, numerical | fixed_size_array);
  EXPECT_EQ(t.array_len, 5);

  t = extract_array_type("bytes[]");
  EXPECT_EQ(t.str, "bytes");
  EXPECT_EQ(t.s_type, dynamic_size_bytes);
  EXPECT_EQ(t.array_len, 0);

  EXPECT_EQ(calculate_offset({"uint256"}), 32);
  EXPECT_EQ(calculate_offset({"uint8", "string", "string[5][]", "bool"}), 8 * 32);
  EXPECT_EQ(calculate_offset({"fixed[2][3][4]"}), 24 * 32);
  EXPECT_EQ(calculate_offset({"string", "bytes"}), 64);
  EXPECT_EQ(calculate_offset({"string[]", "uint32"}), 64);
  EXPECT_EQ(calculate_offset({"bool[3]", "uint256[7][]"}), 320);
  EXPECT_EQ(calculate_offset({"uint256[][10]"}), 32);

  std::string encoded, decoded, signatures, parameters;

  signatures = "[\"bytes32\"]";
  parameters = "[\"A30B12\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint\"]";
  parameters = "[\"02\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint[][]\",\"string[]\"]";
  parameters = "[[[\"01\",\"02\"],[\"03\"]],[\"" + bin2hex("one") + "\",\"" + bin2hex("two") + "\",\""
      + bin2hex("three") + "\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint[]\",\"string\"]";
  parameters = "[[\"01\",\"02\"],\"616161\"]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint\",\"string[2]\"]";
  parameters = "[\"01\",[\"616161\",\"656565\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[2]\"]";
  parameters = "[[\"616161\",\"656565\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2]\"]";
  parameters = "[[[\"616161\",\"656565\"],[\"707070\",\"717171\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2]\",\"uint8[][]\"]";
  parameters = "[[[\"616161\",\"656565\"],[\"707070\",\"717171\"]],[[\"07\",\"08\"],[\"05\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[2][2]\"]";
  parameters = "[[[\"616161\",\"656565\"],[\"707070\",\"717171\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2][]\"]";
  parameters = "[[[[\"616161\"],[\"656565\"]],[[\"707070\"],[\"717171\",\"737373\"]]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[2]\"]";
  parameters = "[[\"01\",\"02\"]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[2][3]\"]";
  parameters = "[[[\"01\",\"02\",\"03\"],[\"05\",\"06\",\"07\"]]]";
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[2][3]\", \"string[][2][]\"]";
  parameters = "[[[\"01\",\"02\",\"03\"],[\"05\",\"06\",\"07\"]],[[[\"616161\"],[\"656565\"]],[[\"707070\"],[\"717171\",\"737373\"]]]]";  // NOLINT
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"string[][2][]\", \"uint256[2][3]\"]";
  parameters = "[[[[\"616161\"],[\"656565\"]],[[\"707070\"],[\"717171\",\"737373\"]]],[[\"01\",\"02\",\"03\"],[\"05\",\"06\",\"07\"]]]";  // NOLINT
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  signatures = "[\"uint256[2][3]\", \"string[][2][2]\"]";
  parameters = "[[[\"01\",\"02\",\"03\"],[\"05\",\"06\",\"07\"]],[[[\"616161\",\"656565\"],[\"717171\",\"737373\"]]]]";  // NOLINT
  encoded = encode(signatures, parameters);
  decoded = decode(signatures, encoded);
  EXPECT_EQ(decoded, parameters);

  return 0;
}
