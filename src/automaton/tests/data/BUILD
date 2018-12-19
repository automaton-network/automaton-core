# Automaton data schema & serialization library tests

package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "protobuf_schema_test",
  srcs = ["protobuf_schema_test.cc",],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
  ],
  data = ["//automaton/tests/data:test.proto"],
  flaky = True,
)

cc_test(
  name = "test_msg_json",
  srcs = ["test_msg_json.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "test_import_dependencies",
  srcs = ["test_import_dependencies.cc",],
  data = [
    "schema1.proto",
    "schema2.proto",
    "schema3.proto",
  ],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "test_protobuf_msg",
  srcs = ["test_protobuf_msg.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "protobuf_schema_definitions",
  srcs = ["protobuf_schema_gtest.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "protobuf_schema_test_find_unexisting_schema",
  srcs = ["protobuf_schema_test_find_unexisting_schema.cc",],
  data = ["//automaton/tests/data:test.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_invalid_data",
  srcs = ["protobuf_schema_test_invalid_data.cc",],
  data = ["//automaton/tests/data:invalid_data.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_find_existing_schema",
  srcs = ["protobuf_schema_test_find_existing_schema.cc",],
  data = ["//automaton/tests/data:test.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_find_all_fields",
  srcs = ["protobuf_schema_test_find_all_fields.cc",],
  data = ["//automaton/tests/data:many_fields.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_find_all_enums",
  srcs = ["protobuf_schema_test_find_all_enums.cc",],
  data = ["//automaton/tests/data:many_enums.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_find_enum",
  srcs = ["protobuf_schema_test_find_enum.cc",],
  data = ["//automaton/tests/data:many_enums.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_empty_schema",
  srcs = ["protobuf_schema_test_empty_schema.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "protobuf_schema_test_setting_fields",
  srcs = ["protobuf_schema_test_setting_fields.cc",],
  data = ["//automaton/tests/data:test.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)

cc_test(
  name = "protobuf_schema_test_message_serialization",
  srcs = ["protobuf_schema_test_message_serialization.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "protobuf_schema_test_enums",
  srcs = ["protobuf_schema_test_enums.cc",],
  deps = [
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
)

cc_test(
  name = "test_protobuf_schema_all_data_types",
  srcs = ["protobuf_schema_all_data_types.cc",],
  data = ["//automaton/tests/data:many_fields.proto"],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
    "@gtest//:gtest_main",
  ],
  size = "small",
  flaky = True,
)
