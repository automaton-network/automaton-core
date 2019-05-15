package(default_visibility = ["//visibility:public"])

cc_library(
  name = "replxx",
  srcs = select({
    "//conditions:windows": ["build/Release/replxx.lib"],
    "//conditions:default": ["build/libreplxx.a"],
  }),
  hdrs = glob([
    "include/*.h",
    "include/*.hxx",
  ]),
  includes = ["include"],
  linkstatic=1,
)
