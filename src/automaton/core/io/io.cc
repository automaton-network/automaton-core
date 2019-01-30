#include "automaton/core/io/io.h"

#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <string>

INITIALIZE_EASYLOGGINGPP

namespace automaton {
namespace core {
namespace io {

bool file_exists(const char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

std::string get_file_contents(const char* filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in) {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  LOG(ERROR) << "Could not read file contents for " << filename;
  throw(errno);
}

static const char* const hex_lut = "0123456789ABCDEF";

std::string bin2hex(const std::string& input) {
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = input[i];
        output.push_back(hex_lut[c >> 4]);
        output.push_back(hex_lut[c & 15]);
    }
    return output;
}

uint8_t hex2nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  throw std::invalid_argument(std::to_string(c) + "not a hex digit!");
}

std::string hex2bin(const std::string& input) {
  size_t len = input.length();
  size_t out_len = (len + 1) / 2;

  std::string output;
  output.reserve(out_len);

  // If input string is an odd length use one nibble to produce an output byte.
  if (len & 1) {
    output.push_back(hex2nibble(input[0]));
  }

  // The rest of the input consists of pair of nibbles.
  for (size_t i = (len & 1); i < len - 1; i += 2) {
    uint8_t n1 = hex2nibble(input[i]);
    uint8_t n2 = hex2nibble(input[i + 1]);
    output.push_back(n1 << 4 | n2);
  }

  return output;
}

std::string get_date_string(std::chrono::system_clock::time_point t) {
  auto as_time_t = std::chrono::system_clock::to_time_t(t);
  struct tm* tm;
  if ((tm = ::gmtime(&as_time_t))) {
    char some_buffer[64];
    if (std::strftime(some_buffer, sizeof(some_buffer), "%F %T", tm)) {
      return std::string{some_buffer};
    }
  }
  throw std::runtime_error("Failed to get current date as string");
}

std::string zero_padded(int num, int width) {
  std::ostringstream ss;
  ss << std::setw(width) << std::setfill('0') << num;
  return ss.str();
}

// *** LOGGING ***


const char* DEFAULT_LOG_CONFIG_FILENAME = "log.cfg";
const char* DEFAILT_LOG_OUTPUT_FILENAME = "automaton-core.log";

bool init_logger() {
  // Load configuration from file
  if (automaton::core::io::file_exists(DEFAULT_LOG_CONFIG_FILENAME)) {
    el::Configurations conf(DEFAULT_LOG_CONFIG_FILENAME);
  } else {
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format,
        std::string("%datetime %levshort [%fbase:%line]: %msg"));

    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "true");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "false");
    el::Loggers::reconfigureAllLoggers(
        el::ConfigurationType::Filename, DEFAILT_LOG_OUTPUT_FILENAME);

    el::Loggers::setLoggingLevel(el::Level::Global);
    // el::Loggers::setVerboseLevel(9);
  }

  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
  el::Loggers::addFlag(el::LoggingFlag::LogDetailedCrashReason);

  return true;
}

bool _init_logger = init_logger();

}  // namespace io
}  // namespace core
}  // namespace automaton
