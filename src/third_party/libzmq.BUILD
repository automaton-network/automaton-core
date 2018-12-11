# Bazel build definitions for locally built libzmq

package(default_visibility = ["//visibility:public"])

cc_library (
  name = "libzmq",
  srcs = glob([
    "src/.libs/libzmq.a",
  ]),
  hdrs = glob([
    "src/**/*.h",
    "src/**/*.hpp",
    "include/**/*.h",
    "include/**/*.hpp",
  ]),
  includes = ["include"],
  deps = [
  ],
  linkstatic = True,
)
