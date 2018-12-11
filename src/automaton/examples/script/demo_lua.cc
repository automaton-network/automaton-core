#include <cctype>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/RIPEMD160_cryptopp.h"
#include "automaton/core/crypto/cryptopp/secure_random_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA3_256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA512_cryptopp.h"
#include "automaton/core/data/msg.h"
#include "automaton/core/data/schema.h"
#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/io/io.h"
#include "automaton/core/script/engine.h"

#include "replxx.hxx"
#include "sol.hpp"

using Replxx = replxx::Replxx;

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::crypto::cryptopp::RIPEMD160_cryptopp;
using automaton::core::crypto::cryptopp::secure_random_cryptopp;
using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::cryptopp::SHA512_cryptopp;
using automaton::core::crypto::cryptopp::SHA3_256_cryptopp;
using automaton::core::crypto::hash_transformation;
using automaton::core::data::msg;
using automaton::core::data::protobuf::protobuf_factory;
using automaton::core::data::protobuf::protobuf_schema;
using automaton::core::data::schema;
using automaton::core::io::get_file_contents;
using automaton::core::io::bin2hex;
using automaton::core::script::engine;

using std::unique_ptr;
using std::make_unique;

protobuf_factory factory;

// prototypes
Replxx::completions_t hook_completion(std::string const& context, int index, void* user_data);
Replxx::hints_t hook_hint(std::string const& context, int index, Replxx::Color& color, void* user_data); // NOLINT
void hook_color(std::string const& str, Replxx::colors_t& colors, void* user_data); // NOLINT

Replxx::completions_t hook_completion(std::string const& context, int index, void* user_data) {
  auto* examples = static_cast<std::vector<std::string>*>(user_data);
  Replxx::completions_t completions;

  std::string prefix {context.substr(index)};
  for (auto const& e : *examples) {
    if (e.compare(0, prefix.size(), prefix) == 0) {
      completions.emplace_back(e.c_str());
    }
  }

  return completions;
}

Replxx::hints_t hook_hint(std::string const& context, int index, Replxx::Color& color, void* user_data) { // NOLINT
  auto* script = static_cast<engine*>(user_data);
  std::vector<std::string> examples;
  Replxx::hints_t hints;

  for (auto i = 0; i < factory.get_schemas_number(); i++) {
    auto name = factory.get_schema_name(i);
    examples.push_back(name);
  }

  // only show hint if prefix is at least 'n' chars long
  // or if prefix begins with a specific character
  std::string prefix {context.substr(index)};
  if (prefix.size() >= 1 || (!prefix.empty() && prefix.at(0) == '.')) {
    for (auto const& e : examples) {
      if (e.compare(0, prefix.size(), prefix) == 0) {
        hints.emplace_back(e.substr(prefix.size()).c_str());
      }
    }

    if (hints.size() == 0) {
      hints.push_back("\nThis is a \x1b[38;5;208mtest\x1b[38;5;15m!!!");
    }
  }

  // set hint color to green if single match found
  if (hints.size() == 1) {
    color = Replxx::Color::GREEN;
  }

  return hints;
}

// NOLINTNEXTLINE
void hook_color(std::string const& context, Replxx::colors_t& colors, void* user_data) {
  auto* regex_color = static_cast<std::vector<std::pair<std::string, Replxx::Color>>*>(user_data);

  // highlight matching regex sequences
  for (auto const& e : *regex_color) {
    size_t pos {0};
    std::string str = context;
    std::smatch match;

    while (std::regex_search(str, match, std::regex(e.first))) {
      std::string c {match[0]};
      pos += std::string(match.prefix()).size();

      for (size_t i = 0; i < c.size(); ++i) {
        colors.at(pos + i) = e.second;
      }

      pos += c.size();
      str = match.suffix();
    }
  }
}

void string_replace(std::string* str,
                    const std::string& oldStr,
                    const std::string& newStr) {
  std::string::size_type pos = 0u;
  while ((pos = str->find(oldStr, pos)) != std::string::npos) {
     str->replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}

struct byte_array {
  explicit byte_array(size_t size) {
    LOG(DEBUG) << "Creating array " << this << " with size " << size;
    buffer.resize(size, 0);
  }

  ~byte_array() {
    LOG(DEBUG) << "Destroying array " << this << " with size " << buffer.size();
  }

  void set(size_t key, int value) {
    if (key >= buffer.size()) {
      throw sol::error("Out of range!");
    }
    LOG(DEBUG) << "Setting key " << key << " to " << value;
    buffer[key] = value;
  }

  int get(size_t key) {
    return buffer[key];
  }

  void resize(size_t new_size) {
    buffer.resize(new_size, 0);
  }

  std::vector<uint8_t> buffer;
};

int main() {
  // Load proto messages
  auto proto_contents = get_file_contents("automaton/examples/script/blockchain.proto");
  auto proto_schema = new protobuf_schema(proto_contents);
  factory.import_schema(proto_schema, "", "");

  engine script(factory);

  auto add_req_id = factory.get_schema_id("AddRequest");
  auto add_rep_id = factory.get_schema_id("AddResponse");

  script.set_function("add", [&](int x, int y) {
    auto req = factory.new_message_by_id(add_req_id);
    auto rep = factory.new_message_by_id(add_rep_id);
    req->set_int32(1, x);
    req->set_int32(2, y);
    rep->set_int32(1, req->get_int32(1) + req->get_int32(2));
    return rep->get_int32(1);
  });

  script.new_usertype<byte_array>("ByteArray",
    sol::constructors<byte_array(size_t)>(),
    sol::meta_function::index, &byte_array::get,
    sol::meta_function::new_index, &byte_array::set,
    sol::meta_function::length, [](byte_array& a) {
      return a.buffer.size();
    },
    sol::meta_function::to_string, [](byte_array& a) {
      return std::string((char*)a.buffer.data(), a.buffer.size()); // NOLINT
    },
    "resize", &byte_array::resize);

  auto random = new secure_random_cryptopp();
  auto ripemd160 = new RIPEMD160_cryptopp();
  auto sha512 = new SHA512_cryptopp();
  auto sha256 = new SHA256_cryptopp();
  auto sha3 = new SHA3_256_cryptopp();
  auto keccak256 = new Keccak_256_cryptopp();

  script.set_function("rand", [random](int bytes) -> std::string {
    uint8_t* buf = new uint8_t[bytes];
    random->block(buf, bytes);
    auto result = std::string((char*)buf, bytes); // NOLINT
    delete[] buf;
    return result;
  });

  script.set_function("fromcpp", [&](const std::string& h, int n) -> std::string {
    hash_transformation* f;
    size_t size = 0;

    if (h == "SHA2-256") {
      f = sha256;
      size = 32;
    }

    if (h == "SHA2-512") {
      f = sha512;
      size = 64;
    }

    if (h == "Keccak-256") {
      f = keccak256;
      size = 32;
    }

    if (h == "SHA3-256") {
      f = sha3;
      size = 32;
    }

    if (h == "RIPEMD-160") {
      f = sha512;
      size = 20;
    }

    if (size > 0) {
      uint8_t* digest1 = new uint8_t[size];
      static uint8_t digest2[64];
      for (uint32_t i = 0; i < n; i++) {
        f->calculate_digest(digest1, i ? size : 0, digest2);
        std::memcpy(digest1, digest2, size);
      }
      delete[] digest1;
      return std::string((char*)digest2, size); // NOLINT
    }
    return "";
  });

  script.set_function("sha256A", [sha256](const std::string& input) -> const std::string {
    static char digest[32];
    sha256->calculate_digest(
        reinterpret_cast<const uint8_t*>(input.c_str()),
        input.size(),
        reinterpret_cast<uint8_t*>(digest));
    return std::string(digest, 32);
  });

  script.set_function("sha256B", [sha256](const char * input, size_t inp_size) -> const char * {
    static char digest[32];
    sha256->calculate_digest(
        reinterpret_cast<const uint8_t*>(input),
        inp_size,
        reinterpret_cast<uint8_t*>(digest));
    return digest;
  });

  script.bind_core();

  // words to be completed
  std::vector<std::string> examples {
    "Point", "Blocks", "BlockHeader",
    "sha3", "sha256", "keccak256", "sha512", "ripemd160"
  };

  // highlight specific words
  // a regex string, and a color
  // the order matters, the last match will take precedence
  using cl = Replxx::Color;
  std::vector<std::pair<std::string, cl>> regex_color {
    // single chars
    {"\\`", cl::BRIGHTCYAN},
    {"\\'", cl::BRIGHTBLUE},
    {"\\\"", cl::BRIGHTBLUE},
    {"\\-", cl::BRIGHTBLUE},
    {"\\+", cl::BRIGHTBLUE},
    {"\\=", cl::BRIGHTBLUE},
    {"\\/", cl::BRIGHTBLUE},
    {"\\*", cl::BRIGHTBLUE},
    {"\\^", cl::BRIGHTBLUE},
    {"\\.", cl::BRIGHTMAGENTA},
    {"\\(", cl::BRIGHTMAGENTA},
    {"\\)", cl::BRIGHTMAGENTA},
    {"\\[", cl::BRIGHTMAGENTA},
    {"\\]", cl::BRIGHTMAGENTA},
    {"\\{", cl::BRIGHTMAGENTA},
    {"\\}", cl::BRIGHTMAGENTA},

    // color keywords
    {"color_black", cl::BLACK},
    {"color_red", cl::RED},
    {"color_green", cl::GREEN},
    {"color_brown", cl::BROWN},
    {"color_blue", cl::BLUE},
    {"color_magenta", cl::MAGENTA},
    {"color_cyan", cl::CYAN},
    {"color_lightgray", cl::LIGHTGRAY},
    {"color_gray", cl::GRAY},
    {"color_brightred", cl::BRIGHTRED},
    {"color_brightgreen", cl::BRIGHTGREEN},
    {"color_yellow", cl::YELLOW},
    {"color_brightblue", cl::BRIGHTBLUE},
    {"color_brightmagenta", cl::BRIGHTMAGENTA},
    {"color_brightcyan", cl::BRIGHTCYAN},
    {"color_white", cl::WHITE},
    {"color_normal", cl::NORMAL},

    // commands
    {"\\.help", cl::BRIGHTMAGENTA},
    {"\\.history", cl::BRIGHTMAGENTA},
    {"\\.quit", cl::BRIGHTMAGENTA},
    {"\\.exit", cl::BRIGHTMAGENTA},
    {"\\.clear", cl::BRIGHTMAGENTA},
    {"\\.prompt", cl::BRIGHTMAGENTA},

    // numbers
    {"[\\-|+]{0,1}[0-9]+", cl::YELLOW},  // integers
    {"[\\-|+]{0,1}[0-9]*\\.[0-9]+", cl::YELLOW},  // decimals
    {"[\\-|+]{0,1}[0-9]+e[\\-|+]{0,1}[0-9]+", cl::YELLOW},  // scientific notation

    // strings
    {"\".*?\"", cl::BRIGHTGREEN},  // double quotes
    {"\'.*?\'", cl::BRIGHTGREEN},  // single quotes

    // crypto
    {"sha3", cl::CYAN},
    {"sha256", cl::CYAN},
    {"sha512", cl::CYAN},
    {"keccak256", cl::CYAN},
    {"ripemd160", cl::CYAN},
  };

  // init the repl
  Replxx rx;
  rx.install_window_change_handler();

  // the path to the history file
  // std::string history_file {"./replxx_history.txt"};

  // load the history file if it exists
  // rx.history_load(history_file);

  // set the max history size
  rx.set_max_history_size(100);

  // set the max input line size
  rx.set_max_line_size(1024);

  // set the max number of hint rows to show
  rx.set_max_hint_rows(12);

  // set the callbacks
  rx.set_completion_callback(hook_completion, static_cast<void*>(&examples));
  rx.set_highlighter_callback(hook_color, static_cast<void*>(&regex_color));
  rx.set_hint_callback(hook_hint, static_cast<void*>(&script));

  static std::string automaton_ascii_logo =
    "\n\x1b[40m\x1b[1m"
    "                                                                   "
    "\x1b[0m\n\x1b[40m\x1b[1m"
    "                                                                   "
    "\x1b[0m\n\x1b[40m\x1b[1m"
    "   @197m█▀▀▀█ @39m█ █ █ @11m▀▀█▀▀ @129m█▀▀▀█ @47m█▀█▀█ @9m█▀▀▀█ @27m▀▀█▀▀ @154m█▀▀▀█ @13m█▀█ █           " // NOLINT
    "\x1b[0m\n\x1b[40m\x1b[1m"
    "   @197m█▀▀▀█ @39m█ ▀ █ @11m█ █ █ @129m█ ▀ █ @47m█ ▀ █ @9m█▀▀▀█ @27m█ █ █ @154m█ ▀ █ @13m█ █ █  @15mCORE     " // NOLINT
    "\x1b[0m\n\x1b[40m\x1b[1m"
    "   @197m▀ ▀ ▀ @39m▀▀▀▀▀ @11m▀ ▀ ▀ @129m▀▀▀▀▀ @47m▀ ▀ ▀ @9m▀ ▀ ▀ @27m▀ ▀ ▀ @154m▀▀▀▀▀ @13m▀ ▀▀▀  @15mv0.0.1   " // NOLINT
    "\x1b[0m\n\x1b[40m\x1b[1m"
    "                                                                   "
    "\x1b[0m\n\n";

  string_replace(&automaton_ascii_logo, "@", "\x1b[38;5;");

/*
  #FF0055 - 197
  #00AAFF - 39
  #FFFF00 - 11, 226
  #AA00FF - 129
  #00FF55 - 47
  #FF0000 - 9, 196
  #0055FF - 27
  #AAFF00 - 154
  #FF00FF - 13, 201
*/

  // display initial welcome message
  std::cout
  << automaton_ascii_logo;

  // set the repl prompt
  std::string prompt {
    // "\x1b[1m\x1b[38;5;15m[A] \x1b[0m"
    "\x1b[38;5;15m"
    "\x1b[48;5;0m"
    // "\x1b[1m"
    "🄰 "
    "\x1b[0m "
  };

/*
  std::thread logger([&]() {
    for (auto i = 0; i < 1000; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      rx.print("Test %d...", i);
    }
  });
*/

  // main repl loop
  for (;;) {
    // display the prompt and retrieve input from the user
    char const* cinput{ nullptr };

    do {
      cinput = rx.input(prompt);
    } while ((cinput == nullptr) && (errno == EAGAIN));

    if (cinput == nullptr) {
      break;
    }

    // change cinput into a std::string
    // easier to manipulate
    std::string input {cinput};

    sol::protected_function_result pfr = script.safe_script(input, &sol::script_pass_on_error);
    std::string output = pfr;
    std::cout << output << std::endl;

    rx.history_add(input);
  }

  // save the history
  // rx.history_save(history_file);

  std::cout << "\nExiting Automaton\n";

  return 0;
}
