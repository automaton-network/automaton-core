#ifndef AUTOMATON_CORE_CLI_CLI_H_
#define AUTOMATON_CORE_CLI_CLI_H_

#include <string>

#include "replxx.hxx"

namespace automaton {
namespace core {
namespace cli {

class cli {
 public:
  // virtual set_prompt(std::string prompt) = 0;
  cli();

  char const* input(const char* prompt);
  void history_add(const char* cmd);
  void print(const char* msg);

 private:
  replxx::Replxx rx;
};

}  // namespace cli
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_CLI_CLI_H_
