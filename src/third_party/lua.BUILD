package(default_visibility = ["//visibility:public"])

cc_library(
    name = "liblua",
    srcs = [
        # Core language
        "lapi.c",
        "lcode.c",
        "lctype.c",
        "ldebug.c",
        "ldo.c",
        "ldump.c",
        "lfunc.c",
        "lgc.c",
        "llex.c",
        "lmem.c",
        "lobject.c",
        "lopcodes.c",
        "lparser.c",
        "lstate.c",
        "lstring.c",
        "ltable.c",
        "ltm.c",
        "lundump.c",
        "lvm.c",
        "lzio.c",

        # Standard libraries
        "lauxlib.c",
        "lbaselib.c",
        "lbitlib.c",
        "lcorolib.c",
        "ldblib.c",
        "liolib.c",
        "lmathlib.c",
        "loslib.c",
        "lstrlib.c",
        "ltablib.c",
        "lutf8lib.c",
        "loadlib.c",
        "linit.c",
    ],
    hdrs = glob(["*.h", "*.hpp"]),
    defines = [
      "LUA_COMPAT_5_2",
      "LUA_COMPAT_MODULE",
    ],
    includes = ["."],
)

cc_binary(
    name = "lua",
    srcs = ["lua.c"],
    deps = [":liblua"],
)

cc_binary(
    name = "luac",
    srcs = ["luac.c"],
    deps = [":liblua"],
)
