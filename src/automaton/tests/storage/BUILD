
cc_test(
  name = "blobstore",
  srcs = ["blobstore_test.cc"],
  size = "small",
  deps = [
    "//automaton/core/storage:blobstore",
    "@gtest//:gtest_main",
  ],
  linkstatic = True,
)

cc_test(
  name = "persistent_blobstore",
  srcs = ["persistent_blobstore_test.cc"],
  size = "small",
  deps = [
    "//automaton/core/storage:persistent_blobstore",
    "//automaton/core/io",
    "@gtest//:gtest_main",
  ],
  linkstatic = True,
  #copts = ["-MT"],
)

#cc_test(
#    name = "persistent_storage",
#    srcs = ["persistent_storage_test.cc"],
#    size = "small",
#    deps = [
#      "//automaton/core/storage:persistent_storage",
#      "//automaton/core/io",
#      "@gtest//:gtest_main",
#    ],
#    #linkstatic = True,
#    #copts = ["-MT"],
#)
