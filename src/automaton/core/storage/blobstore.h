#ifndef AUTOMATON_CORE_STORAGE_BLOBSTORE_H__
#define AUTOMATON_CORE_STORAGE_BLOBSTORE_H__

#include <string>

namespace automaton {
namespace core {
namespace storage {

/**
Blobstore interface.
Can store and delete arbitrary length data in a memory mapped file.
There are no protections. The pointer points to the internal memory.
*/
class blobstore{
 public:
  blobstore();
  ~blobstore();

  /**
  Stores the byte array pointed by data

  @returns    uint64_t  returns the ID used to access it.
  @param[in]  size      The size of the data pointed by data in bytes
  @param[out] data      Pointer to the data
  */
  uint64_t store(const uint32_t size, const uint8_t* data);

  /**
  Used to get access to previously allocated blob.

  @returns    uint8_t*  pointer to the blob or nullptr if id>=capacity
  @param[in]  id        The ID returned by create_blob
  @param[out] size      The size of the data pointed by the returned
  pointer in bytes
  */
  uint8_t* get(const uint64_t id, uint32_t* size);

  /**
  Frees allocated blob.

  @returns    bool      False if there is no allocated blob with the given id
  @param[in]  id        The ID returned by create_blob
  */
  bool free(const uint32_t id);

 private:
  uint32_t* storage;
  std::string file_path;
  uint64_t next_free;
  // capacity of storage in 4byte chunks (size in bytes/4)
  uint64_t capacity;
  // TODO(Samir): Handle free locations.
  // One option is to use heap with the free locations stored by size
  // Second option is to use linked list linking to the next free location
  //  if next.location.ID == this.location.ID + length:
  //    this.location.length += next.location.length
  //  while

  /**
  Creates a blob with a given size

  @returns    uint8_t*  the pointer to the created blob
  @param[in]  size      The size in bytes to be allocated
  @param[out] id        id used to get access to the blob.
  */
  uint8_t* create_blob(uint32_t const size, uint64_t* id);
};

}  // namespace storage
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_STORAGE_BLOBSTORE_H__
