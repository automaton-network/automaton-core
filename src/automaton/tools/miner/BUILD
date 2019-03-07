# Automaton miner

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "miner_lib",
  srcs = [
    "miner.cc",
  ],
  hdrs = [
    "miner.h",
  ],
  deps = [
    "@bitcoin//:secp256k1",
    "//automaton/core/io",
  ],
  linkstatic=True,
)

cc_binary(
  name = "miner_bin",
  srcs = [
    "miner_main.cc",
  ],
  deps = [
    "@bitcoin//:secp256k1",
    "//automaton/core/io",
    "//automaton/tools/miner:miner_lib",
  ],
  linkstatic=True,
)
