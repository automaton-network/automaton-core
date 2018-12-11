# Automaton crypto implementation using crypto++

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "cryptopp",
  srcs = [
    "Keccak_256_cryptopp.cc",
    "RIPEMD160_cryptopp.cc",
    "secp256k1_cryptopp.cc",
    "secure_random_cryptopp.cc",
    "SHA256_cryptopp.cc",
    "SHA3_256_cryptopp.cc",
    "SHA512_cryptopp.cc",
  ],
  hdrs = [
    "Keccak_256_cryptopp.h",
    "RIPEMD160_cryptopp.h",
    "secp256k1_cryptopp.h",
    "secure_random_cryptopp.h",
    "SHA256_cryptopp.h",
    "SHA3_256_cryptopp.h",
    "SHA512_cryptopp.h",
  ],
  deps = [
    "//automaton/core/crypto",
    "@cryptopp//:cryptopp",
  ],
  linkstatic=True,
)
