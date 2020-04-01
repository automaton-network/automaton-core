#include "automaton/core/io/io.h"

#include <sys/stat.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include <g3log/logmessage.hpp>

// INITIALIZE_EASYLOGGINGPP

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
    contents.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  LOG(WARNING) << "Could not read file contents for " << filename;
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
  if (len == 0) {
    return "";
  }
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

std::string dec2hex(uint32_t n) {
  std::stringstream ss;
  ss << std::hex << std::noshowbase << n;
  if (ss.str().size() % 2) {
    return "0" + ss.str();
  }
  return ss.str();
}

uint32_t hex2dec(const std::string& hex) {
  std::stringstream ss(hex);
  uint32_t n;
  ss >> std::hex >> n;
  return n;
}

std::string get_date_string(std::chrono::system_clock::time_point t) {
  auto as_time_t = std::chrono::system_clock::to_time_t(t);
  struct tm tm;
  if (::gmtime_r(&tm, &as_time_t)) {
    char some_buffer[64];
    if (std::strftime(some_buffer, sizeof(some_buffer), "%F %T", &tm)) {
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

struct ColorCoutSink {
// Linux xterm color
// http://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
  enum FG_Color { YELLOW = 33, RED = 31, GREEN = 32, WHITE = 37 };

  FG_Color GetColor(const LEVELS level) const {
     if (level.value == WARNING.value) { return YELLOW; }
     if (level.value == DBUG.value) { return GREEN; }
     if (g3::internal::wasFatal(level)) { return RED; }

     return WHITE;
  }

  void ReceiveLogMessage(g3::LogMessageMover logEntry) {
     auto level = logEntry.get()._level;
     auto color = GetColor(level);

     std::cout << "\033[" << color << "m"
       << logEntry.get().toString() << "\033[m" << std::endl;
  }
};

using g3::LogWorker;

bool init_logger() {
  std::unique_ptr<LogWorker> logworker {LogWorker::createLogWorker()};
  auto sinkHandle = logworker->addSink(std::make_unique<ColorCoutSink>(),
                                   &ColorCoutSink::ReceiveLogMessage);
  g3::initializeLogging(logworker.get());

  return true;
}

}  // namespace io
}  // namespace core
}  // namespace automaton
