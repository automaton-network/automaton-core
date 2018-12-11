# ZMQ examples

package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "asyncsrv",
  srcs = [
    "asyncsrv.cpp",
    "zhelpers.hpp",
  ],
  includes = ["."],
  deps = [
    "@libzmq//:libzmq",
    "@cppzmq//:cppzmq",
  ],
)
