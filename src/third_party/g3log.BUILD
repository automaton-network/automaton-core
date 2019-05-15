package(default_visibility = ["//visibility:public"])

cc_library(
  name = "g3log",
  srcs = select({
    "//conditions:windows": glob(["**/*.lib"]),
    "//conditions:default": glob(["**/*.a"]),
  }),
  hdrs = glob(["**/*.h", "**/*.hpp"]),
  includes = [".", "./src", "./build/include"],
  linkopts = select({
      "//conditions:windows": [],
      "//conditions:default": ["-lpthread"],
  }),
)
