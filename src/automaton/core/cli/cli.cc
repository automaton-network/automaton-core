#include "automaton/core/cli/cli.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using Replxx = replxx::Replxx;

namespace automaton {
namespace core {
namespace cli {

// words to be completed
std::vector<std::string> examples {
  "dump_logs()",
  "localhost",
  "simulation",
  "chat_node",
  "blockchain_node",
  "testnet()",
  "set_mining_power",
  "get_mining_power",
  "Alice",
  "Bob",
  "Charlie",
  "Dave",
  "Emily",
  "random_topology()",
  "start_random_reservations()",
  "stop_random_reservations()",
};

Replxx::completions_t hook_completion(std::string const& context, int index, void* data) {
  Replxx::completions_t completions;

  std::string prefix {context.substr(index)};
  for (auto const& e : examples) {
    if (e.compare(0, prefix.size(), prefix) == 0) {
      completions.emplace_back(e.c_str());
    }
  }

  return completions;
}

// NOLINTNEXTLINE
Replxx::hints_t hook_hint(std::string const& context, int index, Replxx::Color& c, void* data) {
  Replxx::hints_t hints;

  // only show hint if prefix is at least 'n' chars long
  // or if prefix begins with a specific character
  std::string prefix {context.substr(index)};
  if (prefix.size() >= 1) {
    for (auto const& e : examples) {
      if (e.compare(0, prefix.size(), prefix) == 0) {
        hints.emplace_back(e.substr(prefix.size()).c_str());
      }
    }
  }

  // set hint color to green if single match found
  if (hints.size() == 1) {
    c = Replxx::Color::GREEN;
  } else {
    c = Replxx::Color::YELLOW;
  }

  return hints;
}

// NOLINTNEXTLINE
void hook_color(std::string const& str, Replxx::colors_t& colors, void* data) {
}

cli::cli() {
  rx.install_window_change_handler();

  // set the max history size
  rx.set_max_history_size(1000);

  // set the max input line size
  rx.set_max_line_size(1024);

  // set the max number of hint rows to show
  rx.set_max_hint_rows(10);

  // set the callbacks
  rx.set_completion_callback(hook_completion, static_cast<void*>(this));
  rx.set_highlighter_callback(hook_color, static_cast<void*>(this));
  rx.set_hint_callback(hook_hint, static_cast<void*>(this));
}

char const* cli::input(const char* prompt) {
  char const* cinput{ nullptr };

  do {
    cinput = rx.input(prompt);
  } while ((cinput == nullptr) && (errno == EAGAIN));

  return cinput;
}

void cli::history_add(const char* cmd) {
  rx.history_add(cmd);
}

void cli::print(const char * msg) {
  rx.print("%s", msg);
}

}  // namespace cli
}  // namespace core
}  // namespace automaton
