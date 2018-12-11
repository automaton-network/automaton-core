cc_binary(
  name = "protobuf_schema_dump_schema",
  srcs = ["dump_schema.cc",],
  data = [
    "//automaton/tests/data:many_fields.proto",
    "//automaton/tests/data:many_enums.proto",
  ],
  deps = [
    "//automaton/core/io",
    "//automaton/core/data/protobuf",
  ],
)
