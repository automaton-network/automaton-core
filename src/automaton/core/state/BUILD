# Automaton state library

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "state",
  srcs = [
    "state_impl.cc",
    "state_impl.h",
    "state.cc",
    "state.h",
  ],
  hdrs = [
    "state_impl.h",
    "state.h",
  ],
  deps = [
    "//automaton/core/crypto",
    "//automaton/core/io",
  ],
  linkstatic=True,
)

cc_library(
  name = "state_persistent",
  srcs = [
    "state_persistent.cc",
    "state_persistent.h",
    "state.cc",
    "state.h",
  ],
  hdrs = [
    "state_persistent.h",
    "state.h",
  ],
  deps = [
    "//automaton/core/crypto",
    "//automaton/core/io",
    "//automaton/core/storage:persistent_blobstore",
    "//automaton/core/storage:persistent_vector",
  ],
  linkstatic=True,
)
