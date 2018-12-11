# Automaton crypto implementation using orlp/ed25519

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "ed25519_orlp",
  srcs = [
    "ed25519_orlp.cc",
  ],
  hdrs = [
    "ed25519_orlp.h",
  ],
  deps = [
    "//automaton/core/crypto",
    "@ed25519_orlp//:ed25519_orlp",
  ],
  linkstatic=True,
)
