
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "config",
  srcs = glob([
    "boost/config.hpp",
  ]),
  hdrs = glob([
    "boost/*.hpp",
    "boost/config/**/*.hpp",
  ]),
  includes = ["."],
  #linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
)

cc_library(
  name = "algorithm",
  srcs = glob([
    "boost/algorithm/algorithm.hpp",
  ]),
  hdrs = glob([
    "boost/**/*.h",
    "boost/**/*.hpp",
    "boost/**/*.ipp",
  ]),
  includes = ["."],
  #linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
  deps=[
    ":config",
  ],
)

cc_library(
  name = "asio",
  srcs = glob([
    "boost/asio/error.hpp",
  ]),
  hdrs = glob([
    "boost/asio/**/*.ipp",
    "boost/asio/**/*.h*",
  ]),
  includes = ["."],
  #linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
  deps=[
    ":config",
    ":system",
  ],
)

cc_library (
  name = "system",
  srcs = select({
    "//conditions:windows":
      glob(["stage/lib/libboost_system-vc141-mt-x64-*.lib"]),
    "//conditions:default":
      ["stage/lib/libboost_system.a"]
  }),
  hdrs = glob([
    "boost/**/*.h",
    "boost/**/*.hpp",
    "boost/**/*.ipp",
  ]),
  includes = ["."],
  deps = [
    ":config",
  ],
  linkopts = ["-pthread"],
  linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
)

cc_library (
  name = "filesystem",
  srcs = select({
    "//conditions:windows":
      glob(["stage/lib/libboost_filesystem-vc141-mt-x64-*.lib"]),
    "//conditions:default":
      ["stage/lib/libboost_filesystem.a",],
  }),
  hdrs = glob([
    "boost/**/*.hpp"
  ]),
  includes = ["."],
  deps = [
    ":config",
    ":system",
  ],
  linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
)

cc_library (
  name = "iostreams",
  srcs = select({
    "//conditions:windows":
      glob(["stage/lib/libboost_iostreams-vc141-mt-x64-*.lib",]),
    "//conditions:default":
      ["stage/lib/libboost_iostreams.a",],
  }),
  hdrs = glob([
    "boost/**/*.h",
    "boost/**/*.hpp",
  ]),
  includes = ["."],
  deps = [
    ":config",
    ":system",
  ],
  linkstatic = True,
  #defines = ["BOOST_ALL_NO_LIB"],
)
