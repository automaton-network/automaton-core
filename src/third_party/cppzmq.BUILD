# Bazel build definitions for locally built cppzmq

package(default_visibility = ["//visibility:public"])

genrule(
  name = "empty_cc",
  outs = ["empty.cc"],
  cmd = "echo '' > $@",
)

cc_library (
  name = "cppzmq",
  srcs = ["empty.cc"],
  hdrs = glob([
    "**/*.h",
    "**/*.hpp",
  ]),
  includes = ["."],
  linkopts = select({
    "//conditions:linux": [
      "-lpthread",
    ],
    "//conditions:default": [],
  }),
  deps = [
    "@libzmq//:libzmq",
  ],
  linkstatic=1,
)
