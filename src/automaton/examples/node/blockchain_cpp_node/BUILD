package(default_visibility = ["//visibility:public"])

cc_library(
  name = "blockchain_cpp_node",
  srcs = [
    "blockchain_cpp_node.cc",
  ],
  hdrs = [
    "blockchain_cpp_node.h",
  ],
  deps = [
    "//automaton/core/crypto/cryptopp",
    "//automaton/core/data/protobuf",
    "//automaton/core/io",
    "//automaton/core/node:node",
    "//automaton/core/smartproto",
    "@cryptopp//:cryptopp",
  ],
  linkstatic = True,
)

cc_binary(
  name = "blockchain_cpp_node_test",
  srcs = [
    "blockchain_cpp_node_test.cc",
  ],
  deps = [
    ":blockchain_cpp_node",
    "//automaton/core/io",
    "//automaton/core/network:network_tcp",
    "//automaton/core/network:simulated_connection",
    "//automaton/core/node:node_updater",
    "//automaton/core/testnet",
  ]
)

cc_binary(
  name = "blockchain_cpp_node_test_one_mining_thread",
  srcs = [
    "blockchain_cpp_node_test.cc",
  ],
  deps = [
    ":blockchain_cpp_node",
    "//automaton/core/io",
    "//automaton/core/network:network_tcp",
    "//automaton/core/network:simulated_connection",
    "//automaton/core/node:node_updater",
    "//automaton/core/testnet",
  ]
)
