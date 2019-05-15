package(default_visibility = ["//visibility:public"])

genrule(
  name = "empty_cc",
  outs = ["empty.cc"],
  cmd = "echo '#include <json.hpp>' > $@",
)

cc_library(
  name = "json",
  srcs = [
    "empty.cc",
  ],
  hdrs = ["json.hpp"],
  includes = ["."],
  linkstatic=1,
)
