# # Automaton examples - first smart protocol
#
# package(default_visibility = ["//visibility:public"])
#
# cc_library(
#     name = "extended_node_prototype",
#     srcs = [
#       "extended_node_prototype.cc",
#     ],
#     hdrs = [
#       "extended_node_prototype.h",
#     ],
#     data = ["//automaton/examples/network/extended_node:sp.proto"],
#     deps = [
#       "//automaton/core/crypto",
#       "//automaton/core/crypto/cryptopp",
#       "//automaton/core/data/protobuf",
#       "//automaton/core/io",
#       "//automaton/core/state:state",
#       "//automaton/core/network:network",
#       "//automaton/examples/crypto:basic_hash_miner",
#     ],
# )
#
# cc_binary (
# 	name = "simulation_test_final",
# 	srcs = ["simulation_test_final.cc",],
# 	deps = [
#     "//automaton/core/io",
#     "//automaton/core/network:simulated_connection",
#     "//automaton/examples/network/extended_node:extended_node_prototype",
#   ],
# )
#
# cc_binary (
# 	name = "simulation_test_localhost",
# 	srcs = ["simulation_test_localhost.cc",],
# 	deps = [
#     "//automaton/core/io",
#     "//automaton/core/network:network_tcp",
#     "//automaton/examples/network/extended_node:extended_node_prototype",
#   ],
# )
# 
# cc_binary (
# 	name = "simulation_test_localhost_cmd",
# 	srcs = ["simulation_test_localhost_cmd.cc",],
# 	deps = [
#     "//automaton/core/io",
#     "//automaton/core/network:network_tcp",
#     "//automaton/examples/network/extended_node:extended_node_prototype",
#   ],
# )
# 
