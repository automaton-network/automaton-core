package(default_visibility = ["//visibility:public"])

cc_library(
  name = "cryptopp",
  srcs = select({
    "//conditions:windows": glob(["**/*.lib"]),
    "//conditions:default": glob(["**/*.a"]),
  }),
  hdrs = glob(["**/*.h", "**/*.hpp"]),
  includes = ["."],
)

# cc_library(
#   name = "cryptopp",
#   srcs = [
#     "cryptlib.cpp",
#     "cpu.cpp",
#     "integer.cpp",
#     # "3way.cpp",
#     "adler32.cpp",
#     # "algebra.cpp",
#     "algparam.cpp",
#     # "arc4.cpp",
#     # "aria-simd.cpp",
#     # "aria.cpp",
#     # "ariatab.cpp",
#     "asn.cpp",
#     "authenc.cpp",
#     # "base32.cpp",
#     "base64.cpp",
#     "basecode.cpp",
#     "bfinit.cpp",
#     # "blake2-simd.cpp",
#     # "blake2.cpp",
#     # "blowfish.cpp",
#     # "blumshub.cpp",
#     # "camellia.cpp",
#     # "cast.cpp",
#     # "casts.cpp",
#     # "cbcmac.cpp",
#     # "ccm.cpp",
#     # "chacha.cpp",
#     "channels.cpp",
#     # "cmac.cpp",
#     # "crc-simd.cpp",
#     # "crc.cpp",
#     # "default.cpp",
#     "des.cpp",
#     "dessp.cpp",
#     # "dh.cpp",
#     # "dh2.cpp",
#     "dll.cpp",
#     # "dsa.cpp",
#     # "eax.cpp",
#     "ec2n.cpp",
#     # "eccrypto.cpp",
#     "ecp.cpp",
#     # "elgamal.cpp",
#     "emsa2.cpp",
#     # "eprecomp.cpp",
#     # "esign.cpp",
#     "files.cpp",
#     "filters.cpp",
#     "fips140.cpp",
#     # "fipstest.cpp",
#     # "gcm-simd.cpp",
#     # "gcm.cpp",
#     # "gf256.cpp",
#     # "gf2_32.cpp",
#     "gf2n.cpp",
#     "gfpcrypt.cpp",
#     # "gost.cpp",
#     # "gzip.cpp",
#     "hex.cpp",
#     "hmac.cpp",
#     "hrtimer.cpp",
#     # "ida.cpp",
#     # "idea.cpp",
#     "iterhash.cpp",
#     # "kalyna.cpp",
#     # "kalynatab.cpp",
#     "keccak.cpp",
#     # "luc.cpp",
#     # "mars.cpp",
#     # "marss.cpp",
#     # "md2.cpp",
#     # "md4.cpp",
#     # "md5.cpp",
#     "misc.cpp",
#     "modes.cpp",
#     "mqueue.cpp",
#     # "mqv.cpp",
#     "nbtheory.cpp",
#     # "neon-simd.cpp",
#     # "network.cpp",
#     "oaep.cpp",
#     "osrng.cpp",
#     # "padlkrng.cpp",
#     # "panama.cpp",
#     "pkcspad.cpp",
#     # "poly1305.cpp",
#     # "polynomi.cpp",
#     # "ppc-simd.cpp",
#     "pssr.cpp",
#     "pubkey.cpp",
#     "queue.cpp",
#     # "rabin.cpp",
#     "randpool.cpp",
#     # "rc2.cpp",
#     # "rc5.cpp",
#     # "rc6.cpp",
#     # "rdrand.cpp",
#     "rdtables.cpp",
#     # "rijndael-simd.cpp",
#     "rijndael.cpp",
#     "ripemd.cpp",
#     "rng.cpp",
#     # "rsa.cpp",
#     # "rw.cpp",
#     # "safer.cpp",
#     # "salsa.cpp",
#     # "seal.cpp",
#     # "seed.cpp",
#     # "serpent.cpp",
#     # "sha-simd.cpp",
#     "sha.cpp",
#     "sha3.cpp",
#     # "shacal2-simd.cpp",
#     # "shacal2.cpp",
#     # "shark.cpp",
#     # "sharkbox.cpp",
#     # "simon-simd.cpp",
#     # "simon.cpp",
#     # "skipjack.cpp",
#     # "sm3.cpp",
#     # "sm4.cpp",
#     # "socketft.cpp",
#     # "sosemanuk.cpp",
#     # "speck-simd.cpp",
#     # "speck.cpp",
#     # "square.cpp",
#     # "squaretb.cpp",
#     "sse-simd.cpp",
#     # "strciphr.cpp",
#     # "tea.cpp",
#     "tftables.cpp",
#     # "threefish.cpp",
#     # "tiger.cpp",
#     # "tigertab.cpp",
#     # "trdlocal.cpp",
#     # "ttmac.cpp",
#     # "tweetnacl.cpp",
#     # "twofish.cpp",
#     # "vmac.cpp",
#     # "wait.cpp",
#     # "wake.cpp",
#     # "whrlpool.cpp",
#     "xtr.cpp",
#     # "xtrcrypt.cpp",
#     "zdeflate.cpp",
#     "zinflate.cpp",
#     # "zlib.cpp",
#   ],
#   hdrs =
#     glob([
#       "*.h",
#       "*.cpp",
#     ]),
#   includes = ["."],
#   defines = [
#     "CRYPTOPP_DISABLE_ASM",
#     "CRYPTOPP_DISABLE_AESNI",
#     "CRYPTOPP_DISABLE_SHA",
#     "CRYPTOPP_DISABLE_SSSE3",
#     "CRYPTOPP_DISABLE_SSE4",
#   ],
#   copts = [
# #    "-g2",
# #    "-O3",
# #    "-fPIC",
# #    "-pthread",
# #    "-pipe",
# #    "-msha",
# #    "-mpclmul",
# #    "-mpopcnt",
# #    "-maes",
# #    "-msse4.2",
# #    "-c",
#   ],
#   # linkopts = [
#   #   '-lm',
#   # ],
#   linkstatic=1,
# )
#
