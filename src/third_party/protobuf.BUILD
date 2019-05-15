package(default_visibility = ["//visibility:public"])

cc_library(
  name = "protobuf",
  srcs = select({
    "//conditions:windows": glob(["**/libprotobuf*.lib"]),
    "//conditions:default": glob(["**/*.a"]),
  }),
  hdrs = glob(["**/*.h", "**/*.hpp"]),
  includes = ["src"],
)
