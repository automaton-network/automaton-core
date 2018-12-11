# Automaton examples - crypto library

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "basic_hash_miner",
    srcs = [
      "basic_hash_miner.cc",
      "basic_hash_miner.h",
    ],
    deps = [
      "//automaton/core/crypto",
      "//automaton/core/crypto/cryptopp",
    ],
)

cc_binary(
    name = "miner_demo",
    srcs = [
      "miner_demo.cc",
    ],
    deps = [
      "//automaton/core/crypto",
      "//automaton/core/crypto/cryptopp",
      "//automaton/examples/crypto:basic_hash_miner"
    ],
)
