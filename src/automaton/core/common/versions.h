#ifndef AUTOMATON_CORE_COMMON_VERSIONS_H_
#define AUTOMATON_CORE_COMMON_VERSIONS_H_

#include <string>

#define AUTOMATON_CORE_VERSION_MAJOR 0
#define AUTOMATON_CORE_VERSION_MINOR 1
#define AUTOMATON_CORE_VERSION_REVISION 20

namespace automaton {
namespace core {
namespace common {

class versions {
 public:
  static unsigned automaton_core_major() {
    return AUTOMATON_CORE_VERSION_MAJOR;
  }
  static unsigned automaton_core_minor() { return AUTOMATON_CORE_VERSION_MINOR; }
  static unsigned automaton_core_revision() { return AUTOMATON_CORE_VERSION_REVISION; }

  static std::string automaton_core_version_string() {
    return
        std::to_string(automaton_core_major()) + "." +
        std::to_string(automaton_core_minor()) + "." +
        std::to_string(automaton_core_revision());
  }
};

}  // namespace common
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_COMMON_VERSIONS_H_
