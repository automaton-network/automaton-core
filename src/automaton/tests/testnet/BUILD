cc_test (
  name = "testnet_localhost",
  srcs = ["test_localhost.cc",],
  size = "medium",
  data = ["testproto",],
  deps = [
    "//automaton/core/network:network_tcp",
    "//automaton/core/smartproto",
    "//automaton/core/node",
    "//automaton/core/testnet",
    "@gtest//:gtest_main",
  ],
)
cc_test (
  name = "testnet_simulation",
  srcs = ["test_simulation.cc",],
  size = "medium",
  data = ["testproto",],
  deps = [
    "//automaton/core/network:simulated_connection",
    "//automaton/core/smartproto",
    "//automaton/core/node",
    "//automaton/core/testnet",
    "@gtest//:gtest_main",
  ],
)
