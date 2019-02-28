# Automaton miner

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "miner",
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
