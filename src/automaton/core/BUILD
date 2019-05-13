# Automaton Core

package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "core",
  srcs = [
    "core.cc",
  ],
  deps = [
    "//automaton/core/cli",
    "//automaton/core/data",
    "//automaton/core/io",
    "//automaton/core/network",
    "//automaton/core/network:network_tcp",
    "//automaton/core/network:http_server",
    "//automaton/core/network:simulated_connection",
    "//automaton/core/smartproto",
    "//automaton/core/node",
    "//automaton/core/node:node_updater",
    "//automaton/core/node/lua_node",
    "//automaton/core/state",
    "//automaton/core/testnet",
    "@cryptopp//:cryptopp",
    "@json//:json",
  ],
  linkstatic = True,
)

cc_binary(
  name = "rpc_test",
  srcs = [
    "rpc_server_core_test.cc",
  ],
  deps = [
    "//automaton/core/network:network_tcp",
  ],
  linkstatic = True,
)
