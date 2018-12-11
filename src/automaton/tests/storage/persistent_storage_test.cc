#include "automaton/core/storage/persistent_storage.h"
#include <cstring>
#include <random>
#include <vector>
#include <iostream>
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::storage::persistent_blobstore;

TEST(persistent_blobstore, create_mapped_file) {
  std::vector<uint64_t> ids;
  std::vector<std::uint64_t> data;
  data.push_back(1);
  data.push_back(22);
  data.push_back(333);
  data.push_back(4444);
  data.push_back(55555);
  data.push_back(666666);
  data.push_back(7777777);
  data.push_back(88888888);
  data.push_back(999999999);
  {
    persistent_storage storage1;
    storage1.map_file("mapped_file_with_fixed_size_data.txt", sizeof(uint64_t));
    for (int i = 0; i < data.size(); i++) {
      storage1.store(data[i].size(), reinterpret_cast<const uint8_t*>(data[i].data()));
    }
  }
  uint32_t sz;
  uint8_t* pData;
  persistent_storage storage1;
  storage1.map_file("mapped_file_with_fixed_size_data.txt");
  for (int i = 0; i < data.size(); i++) {
    pData = storage1.get(ids[i], &sz);
    std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
    EXPECT_FALSE(std::memcmp(pData, &data[i][0], sz));
  }
  // for (int i = 0; i < 6; i++) {
  //   pData = storage1.get(ids[i], &sz);
  //   std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
  // }
}
