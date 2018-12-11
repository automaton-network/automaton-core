#ifndef AUTOMATON_CORE_STORAGE_PERSISTENT_VECTOR_H__
#define AUTOMATON_CORE_STORAGE_PERSISTENT_VECTOR_H__

#include <string>
#include "automaton/core/storage/persistent_storage.h"

namespace automaton {
namespace core {
namespace storage {

/**
  Vector like storage using memory mapped file.
*/
template<typename T>
class persistent_vector : protected persistent_storage {
 public:
  T& operator[](size_t n);
  void push_back(const T& val);
  bool map_file(std::string path);
  size_t size() const;
  void resize(size_t n);
 private:
  // header[8] is used to store next_free in the mapped file;
  size_t next_free;
};

template<typename T>
inline T & persistent_vector<T>::operator[](size_t n) {
  if (!mapped()) {
    throw "not mapped";
  }
  return *(reinterpret_cast<T*>(get(n)));
}

template<typename T>
void persistent_vector<T>::push_back(const T & val) {
  if (!mapped()) {
    throw "not mapped";
  }
  store(next_free, reinterpret_cast<const uint8_t*>(&val));
  next_free++;
  header[8]++;
}

template<typename T>
inline bool persistent_vector<T>::map_file(std::string path) {
  if (!persistent_storage::map_file(path, sizeof(T))) {
    throw "could not map file";
  }
  next_free = header[8];
}

template<typename T>
size_t persistent_vector<T>::size() const {
  return next_free;
}

template<typename T>
void persistent_vector<T>::resize(size_t n) {
  next_free = n;
  header[8] = n;
}

}  // namespace storage
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_STORAGE_PERSISTENT_VECTOR_H__
