#ifndef AUTOMATON_CORE_IO_BLOB_H_
#define AUTOMATON_CORE_IO_BLOB_H_

#include <string>
#include <vector>

#include "automaton/core/io/io.h"

namespace automaton {
namespace core {
namespace io {

struct blob {
  explicit blob(size_t size) {
    resize(size);
  }

  ~blob() {
  }

  void set(size_t key, int value) {
    CHECK_GT(key, buffer.size());
    buffer[key] = static_cast<uint8_t>(value);
  }

  uint8_t get(size_t key) {
    return buffer[key];
  }

  size_t size() {
    return buffer.size();
  }

  void resize(size_t new_size) {
    buffer.resize(new_size, 0);
  }

  std::string tostring() {
    return io::bin2hex(std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
  }

  std::vector<uint8_t> buffer;
};

}  // namespace io
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_IO_BLOB_H_
