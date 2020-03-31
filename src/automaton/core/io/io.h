#ifndef AUTOMATON_CORE_IO_IO_H__
#define AUTOMATON_CORE_IO_IO_H__

#include <chrono>
#include <string>

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>


// Backward compatibility with easylogging++

#define CHECK_NOTNULL(x) CHECK(x != nullptr)
#define CHECK_BOUNDS(x, xlo, xhi) CHECK((x >= xlo) && (x <= xhi))
#define VLOG(x) LOG(INFO)
#define CHECK_LT(x, y) CHECK(x < y)
#define CHECK_GT(x, y) CHECK(x > y)

// TODO(asen): do a better stacktrace

namespace el {
namespace base {
namespace debug {

inline std::string StackTrace() { return ""; }

}
}
}

namespace automaton {
namespace core {
namespace io {

/**
  Checks whether the specified file exist.

  Returns true if it does.
*/
bool file_exists(const char* filename);

/**
  Gets the file contents and returns them as a string.

  Exception is thrown when error occurs.
*/
std::string get_file_contents(const char* filename);

/**
  Converts binary byte buffer to hex string representation.
*/
std::string bin2hex(const std::string& input);

/**
  Creates a binary byte buffer from hex string representation.
*/
std::string hex2bin(const std::string& input);

/**
  Returns string representing hex value of a number. Does not contain base prefix 0x. Length is always even.
*/
std::string dec2hex(uint32_t n);

/**
  Returns a number from its hex representation.
*/
uint32_t hex2dec(const std::string& hex);


/**
  Returns string representation of a date.
*/
std::string get_date_string(std::chrono::system_clock::time_point t);

std::string zero_padded(int num, int width);

// This must be called after main entry point.
extern bool init_logger();

}  // namespace io
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_IO_IO_H__
