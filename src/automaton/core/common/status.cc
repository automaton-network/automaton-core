#include "automaton/core/common/status.h"

#include <sstream>

namespace automaton {
namespace core {
namespace common {

std::string status::to_string() const {
  std::stringstream ss;
  ss << status(*this);
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const status& s) {
  switch (s.code) {
    case status::OK: os << "OK"; break;
    case status::CANCELLED: os << "CANCELLED"; break;
    case status::UNKNOWN: os << "UNKNOWN"; break;
    case status::INVALID_ARGUMENT: os << "INVALID_ARGUMENT"; break;
    case status::DEADLINE_EXCEEDED: os << "DEADLINE_EXCEEDED"; break;
    case status::NOT_FOUND: os << "NOT_FOUND"; break;
    case status::ALREADY_EXISTS: os << "ALREADY_EXISTS"; break;
    case status::PERMISSION_DENIED: os << "PERMISSION_DENIED"; break;
    case status::UNAUTHENTICATED: os << "UNAUTHENTICATED"; break;
    case status::RESOURCE_EXHAUSTED: os << "RESOURCE_EXHAUSTED"; break;
    case status::FAILED_PRECONDITION: os << "FAILED_PRECONDITION"; break;
    case status::ABORTED: os << "ABORTED"; break;
    case status::OUT_OF_RANGE: os << "OUT_OF_RANGE"; break;
    case status::UNIMPLEMENTED: os << "UNIMPLEMENTED"; break;
    case status::INTERNAL: os << "INTERNAL"; break;
    case status::UNAVAILABLE: os << "UNAVAILABLE"; break;
    case status::DATA_LOSS: os << "DATA_LOSS"; break;
    default: os << "UNKNOWN STATUS";
  }
  if (s.msg != "") {
    os << ": " << s.msg;
  }
  return os;
}

}  // namespace common
}  // namespace core
}  // namespace automaton
