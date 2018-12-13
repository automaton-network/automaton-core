# Automaton Core Node

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "node",
  srcs = [
    "node.cc",
  ],
  hdrs = [
    "node.h",
  ],
  deps = [
    "@localboost//:algorithm",
    "//automaton/core/data",
    "//automaton/core/data/protobuf",
    "//automaton/core/io",
    "//automaton/core/network",
    "//automaton/core/script",
    "//automaton/core/smartproto",
    "@json//:json",
  ],
  linkstatic=True,
  defines = select({
      "//conditions:windows": ["BOOST_ALL_NO_LIB"],
      "//conditions:default": [],
  }),
)
