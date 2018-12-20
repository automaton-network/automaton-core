
cc_test(
  name = "state",
  srcs = ["state_test.cc"],
  size = "medium",
  timeout = "moderate",
  deps = [
    "//automaton/core/crypto",
    "//automaton/core/crypto/cryptopp",
    "//automaton/core/state",
    "@gtest//:gtest_main",
  ],
  flaky = True,
)

cc_test(
  name = "state_persistent",
  srcs = ["state_persistent_test.cc"],
  size = "medium",
  timeout = "moderate",
  deps = [
    "//automaton/core/crypto",
    "//automaton/core/crypto/cryptopp",
    "//automaton/core/state:state_persistent",
    "//automaton/core/io",
    "@gtest//:gtest_main",
  ],
  flaky = True,
)
