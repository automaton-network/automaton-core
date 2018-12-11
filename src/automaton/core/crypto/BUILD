# Automaton crypto module interface

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "crypto",
  srcs = [
    "hash_transformation.cc",
    "digital_signature.cc",
    "secure_random.cc",
  ],
  hdrs = [
    "hash_transformation.h",
    "digital_signature.h",
    "secure_random.h",
  ],
  deps = [
    "//automaton/core/data",
    "//automaton/core/io",
  ],
  linkstatic=True,
)
