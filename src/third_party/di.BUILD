package(default_visibility = ["//visibility:public"])

genrule(
  name = "empty_cc",
  outs = ["empty.cc"],
  cmd = "echo '' > $@",
)

cc_library(
  name = "di",
  srcs = [
    "empty.cc",
    "boost/di.hpp"
  ],
  hdrs = [
    "boost/di.hpp",
  ],
  includes = ["."],
  copts = [
    "--std=c++14",
  ],
)
