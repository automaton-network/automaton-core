#include "automaton/core/storage/persistent_blobstore.h"
#include <cstring>
#include <random>
#include <vector>
#include <iostream>
#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::storage::persistent_blobstore;

TEST(persistent_blobstore, create_mapped_file) {
  std::vector<uint64_t> ids;
  std::vector<std::string> data;
  data.push_back("data 1");
  data.push_back("data 2, data 2");
  data.push_back("data 3, data 3, data 3");
  data.push_back("data 4, data 4, data 4, data 4");
  data.push_back("data 5, data 5, data 5, data 5, data 5");
  data.push_back("data 6, data 6, data 6, data 6, data 6, data 6");
  data.push_back("data 7, data 7, data 7, data 7, data 7, data 7, data 7");
  data.push_back("data 8, data 8, data 8, data 8, data 8, data 8, data 8, data 8");
  data.push_back("data 9, data 9, data 9, data 9, data 9, data 9, data 9, data 9, data 9");
  {
    persistent_blobstore bs1;
    bs1.map_file("build/mapped_file.txt");
    for (size_t i = 0; i < data.size(); i++) {
      ids.push_back(bs1.store(data[i].size(), reinterpret_cast<const uint8_t*>(data[i].data())));
    }
  }
  uint32_t sz;
  uint8_t* pData;
  persistent_blobstore bs1;
  bs1.map_file("build/mapped_file.txt");
  for (size_t i = 0; i < data.size(); i++) {
    pData = bs1.get(ids[i], &sz);
    std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
    EXPECT_FALSE(std::memcmp(pData, &data[i][0], sz));
  }
  // for (int i = 0; i < 6; i++) {
  //   pData = bs1.get(ids[i], &sz);
  //   std::cout << std::string(reinterpret_cast<char*>(pData), sz) << std::endl;
  // }
}
