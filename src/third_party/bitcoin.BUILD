package(default_visibility = ["//visibility:public"])

cc_library(
  name = "secp256k1",
  srcs = glob(["src/secp256k1/src/secp256k1.c"]),
  hdrs = glob([
    "src/secp256k1/**/*.h",
  ]),
  includes = [
    "./src",
    "./src/secp256k1",
    "./src/secp256k1/src",
    "./src/secp256k1/include",
  ],
  defines = [
    "USE_ASM_X86_64",
    "USE_ENDOMORPHISM",
    "USE_FIELD_10X26",
    "USE_FIELD_INV_BUILTIN",
    "USE_SCALAR_INV_BUILTIN",
    "USE_FIELD_INV_NUM",
    "USE_SCALAR_8X32",
    "ENABLE_MODULE_RECOVERY",
    "USE_NUM_NONE",
  ],
)
