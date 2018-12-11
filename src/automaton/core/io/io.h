#ifndef AUTOMATON_CORE_IO_IO_H__
#define AUTOMATON_CORE_IO_IO_H__

#include <easylogging++.h>

#include <string>

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

/// Ensures initialization of the logger prior to calling main(...)
extern bool _init_logger;

}  // namespace io
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_IO_IO_H__
