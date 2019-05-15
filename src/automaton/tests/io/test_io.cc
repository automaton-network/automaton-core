#include "automaton/core/io/io.h"
#include "gtest/gtest.h"

using automaton::core::io::get_file_contents;

const char* TEST_TXT_CONTENTS = "Hello, World!";

TEST(get_file_contents, success) {
  std::string test_txt = get_file_contents("automaton/tests/io/test.txt");
  EXPECT_EQ(test_txt, TEST_TXT_CONTENTS);
}

TEST(get_file_contents, invalid_file_name) {
  try {
    std::string test_txt = get_file_contents("invalid_file_name");
    FAIL() << "We should have gotten an exception when trying to load a missing file.";
  } catch(...) {
  }
}
