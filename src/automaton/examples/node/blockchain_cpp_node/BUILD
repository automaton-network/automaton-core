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
