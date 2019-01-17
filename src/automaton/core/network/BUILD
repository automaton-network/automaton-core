# Automaton network library

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "network",
  srcs = [
    "acceptor.cc",
    "connection.cc",
  ],
  hdrs = [
    "acceptor.h",
    "connection.h",
  ],
  deps = [
    "//automaton/core/common:status",
  ],
  linkstatic=True,
)

cc_library(
  name = "network_tcp",
  srcs = [
    "tcp_implementation.h",
    "tcp_implementation.cc",
  ],
  deps = [
    "@localboost//:asio",
    ":network",
    "//automaton/core/io",
  ],
  linkstatic=True,
  defines = select({
      "//conditions:windows": ["BOOST_ALL_NO_LIB"],
      "//conditions:default": [],
  }),
)

# cc_library(
#   name = "network_zmq",
#   srcs = [
#     "zmq_connection.h",
#     "zmq_connection.cc",
#   ],
#   deps = [
#     "@libzmq//:libzmq",
#     "@cppzmq//:cppzmq",
#     ":network",
#     "//automaton/core/io",
#   ],
#   linkstatic=True,
# )

cc_library(
  name = "simulated_connection",
  srcs = [
    "simulated_connection.h",
    "simulated_connection.cc",
  ],
  deps = [
    ":network",
    "//automaton/core/io",
  ],
  linkopts = select({
    "//conditions:linux": ["-lpthread",],
    "//conditions:default": [],
  }),
  linkstatic=True,
)

#
# cc_library(
#   name = "rpc_server",
#   srcs = [
#     "rpc.h",
#     "rpc.cc",
#   ],
#   deps = [
#     "@localboost//:asio",
#     "//automaton/core/io",
#   ],
#   linkopts = select({
#     "//conditions:linux": ["-lpthread",],
#     "//conditions:default": [],
#   }),
#   linkstatic=True,defines = select({
#       "//conditions:windows": ["BOOST_ALL_NO_LIB"],
#       "//conditions:default": [],
#   }),
# )
