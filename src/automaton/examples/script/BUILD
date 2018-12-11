# Automaton examples - Lua integration

package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "demo_lua",
  srcs = [
    "demo_lua.cc",
  ],
  deps = [
    "@replxx//:replxx",
    "//automaton/core/crypto",
    "//automaton/core/data/protobuf",
    "//automaton/core/io",
    "//automaton/core/script",
  ],
  data = [
    "benchmark.lua",
    "blockchain.lua",
    "blockchain.proto",
    "data.lua",
    "script_demo.lua",
  ],
)
