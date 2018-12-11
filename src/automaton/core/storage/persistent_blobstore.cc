#include "automaton/core/storage/persistent_blobstore.h"
#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>

namespace automaton {
namespace core {
namespace storage {

persistent_blobstore::persistent_blobstore()
    : next_free(1024/4)
    , capacity(1024)
    , header_size(1024)
    , cur_version(10000)
    , is_mapped(false) {
  // TODO(Samir): Remove from constructor and do it when creating blob
  // storage = new uint32_t[1ULL << 28];
  // capacity = 1ULL << 28;
}

persistent_blobstore::~persistent_blobstore() {
  mmf.close();
}

uint8_t* persistent_blobstore::create_blob(const uint32_t size, uint64_t* id) {
  if (is_mapped == false) {
    throw std::logic_error("not mapped");;
  }

  uint32_t size_in_int32 =  size % 4 ? size / 4 + 1 : size / 4;
  *id = next_free;
  // When the capacity is not large enough to store the required blob,
  // allocate a new memory block with double the size and copy the data.
  if (next_free + size_in_int32 >= capacity) {
    close_mapped_file();
    capacity *= 2;
    boost::filesystem::path boost_path(file_path);
    boost::filesystem::resize_file(boost_path, capacity*sizeof(int32_t));
    open_mapped_file();
  }
  // Save the size of the blob
  storage[next_free] = size;
  // set the pointer to point to the blob
  uint8_t* out_blob_pointer = reinterpret_cast<uint8_t*>(&storage[next_free+1]);
  // next free equal to byte after the end of the blob
  next_free += size_in_int32 + 1;

  return out_blob_pointer;
}

uint64_t persistent_blobstore::store(const uint32_t size, const uint8_t* data) {
  if (is_mapped == false) {
    throw std::logic_error("not mapped");;
  }

  uint64_t id = 0;
  uint8_t* blob = create_blob(size, &id);
  std::memcpy(blob, data, size);
  return id;
}

uint8_t* persistent_blobstore::get(const uint64_t id, uint32_t* size) {
  if (is_mapped == false) {
    throw std::logic_error("not mapped");;
  }

  // check if id is out of range
  if (id+1 >= capacity) {
    throw std::out_of_range("Object out of range");
  }
  *size = storage[id];
  if (*size <= 0) {
    return nullptr;
  }
  return reinterpret_cast<uint8_t*>(&storage[id + 1]);
}

bool persistent_blobstore::free(const uint32_t id) {
  if (is_mapped == false) {
    throw std::logic_error("not mapped");;
  }
  if (id+1 >= capacity) {
    throw std::out_of_range("Object out of range");
  }
  // TODO(Samir): Change storage to unt8_t. Mark deleted nodes with *= -1
  storage[id] *= -1;
  return 1;
}

bool persistent_blobstore::map_file(std::string path) {
  if (mmf.is_open() || is_mapped) {
    return false;
  }

  file_path = path;

  if (boost::filesystem::exists(path)) {
    mmf.open(path, boost::iostreams::mapped_file::mapmode::readwrite);
    storage = reinterpret_cast<uint32_t*>(mmf.data());
    capacity = mmf.size() / 4;
    memcpy(&header_version, storage, sizeof(uint64_t));
  } else {
    boost::iostreams::mapped_file_params new_mmf(path);
    new_mmf.flags = boost::iostreams::mapped_file::mapmode::readwrite;
    // The starting size is 2KB by default, we should add a option set the starting size
    new_mmf.new_file_size = 1ULL << 10;
    capacity = new_mmf.new_file_size / 4;
    mmf.open(new_mmf);
    // write the version to the header of the new file
    storage = reinterpret_cast<uint32_t*>(mmf.data());
    uint64_t* header = reinterpret_cast<uint64_t*>(storage);
    header_version = cur_version;
    *header = header_version;
  }
  is_mapped = true;
  return true;
}

void persistent_blobstore::close_mapped_file() {
  mmf.close();
  is_mapped = false;
}

void persistent_blobstore::open_mapped_file() {
  mmf.open(file_path, boost::iostreams::mapped_file::mapmode::readwrite);
  storage = reinterpret_cast<uint32_t*>(mmf.data());
  capacity = mmf.size() / 4;
  is_mapped = true;
}

}  //  namespace storage
}  //  namespace core
}  //  namespace automaton
