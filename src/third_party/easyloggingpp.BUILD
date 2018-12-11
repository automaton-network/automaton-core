package(default_visibility = ["//visibility:public"])

cc_library(
  name = "easyloggingpp",
  srcs = [
    "easylogging++.cc",
    "easylogging++.h",
  ],
  hdrs = [
    "easylogging++.h",
  ],
  includes = ["."],
  defines = [
    "ELPP_NO_LOG_TO_FILE",
    "ELPP_NO_DEFAULT_LOG_FILE",
    "ELPP_FEATURE_PERFORMANCE_TRACKING",
    "ELPP_FEATURE_CRASH_LOG",
    "ELPP_THREAD_SAFE",
    "ELPP_STL_LOGGING",
  ],
  linkstatic=True,
)
