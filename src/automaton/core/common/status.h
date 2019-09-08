#ifndef AUTOMATON_CORE_COMMON_STATUS_H_
#define AUTOMATON_CORE_COMMON_STATUS_H_

#include <ostream>
#include <string>
#include <iostream>

namespace automaton {
namespace core {
namespace common {

struct status {
  enum status_code {
    OK = 0,
    CANCELLED = 1,
    UNKNOWN = 2,
    INVALID_ARGUMENT = 3,
    DEADLINE_EXCEEDED = 4,
    NOT_FOUND = 5,
    ALREADY_EXISTS = 6,
    PERMISSION_DENIED = 7,
    UNAUTHENTICATED = 16,
    RESOURCE_EXHAUSTED = 8,
    FAILED_PRECONDITION = 9,
    ABORTED = 10,
    OUT_OF_RANGE = 11,
    UNIMPLEMENTED = 12,
    INTERNAL = 13,
    UNAVAILABLE = 14,
    DATA_LOSS = 15,
  };

  explicit status(status_code error_code) : code(error_code), msg("") {}
  status(status_code error_code, std::string msg) : code(error_code), msg(msg) {}

  status_code code;
  std::string msg;

  static status ok() {
    return status(OK);
  }

  static status ok(std::string msg) {
    return status(OK, msg);
  }

  static status canceled(std::string msg) {
    return status(CANCELLED, msg);
  }

  static status invalid_argument(std::string msg) {
    return status(INVALID_ARGUMENT, msg);
  }

  static status unknown(std::string msg) {
    return status(UNKNOWN, msg);
  }

  static status not_found(std::string msg) {
    return status(NOT_FOUND, msg);
  }

  static status permission_denied(std::string msg) {
    return status(PERMISSION_DENIED, msg);
  }

  static status resource_exhausted(std::string msg) {
    return status(RESOURCE_EXHAUSTED, msg);
  }

  static status failed_precondition(std::string msg) {
    return status(FAILED_PRECONDITION, msg);
  }

  static status aborted(std::string msg) {
    return status(ABORTED, msg);
  }

  static status out_of_range(std::string msg) {
    return status(OUT_OF_RANGE, msg);
  }

  static status unimplemented(std::string msg) {
    return status(UNIMPLEMENTED, msg);
  }

  static status internal(std::string msg) {
    return status(INTERNAL, msg);
  }

  static status unavailable(std::string msg) {
    return status(UNAVAILABLE, msg);
  }

  static status data_loss(std::string msg) {
    return status(DATA_LOSS, msg);
  }

  std::string to_string() const;
};

std::ostream& operator<<(std::ostream& os, const status& s);

}  // namespace common
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_COMMON_STATUS_H_
