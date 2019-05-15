#include "automaton/tests/data/proto_files.h"

const char * INVALID_DATA_PROTO = R"(
syntax = "proto3";

message TestMsg {
	string name = 1;
  map<string, int32> projects = 13;
	int32 number = 2;
	fixed32 num = 5;
	int32 opt = 3;
	repeated string array = 4;
}
)";

const char * MANY_ENUMS_PROTO = R"(
syntax = "proto3";

message TestMsg {
	string name = 1;
	int32 number = 2;
	int32 num = 5;
	int32 opt = 3;
	repeated string array = 4;
	enum enum1 {
		a0 = 0;
		b0 = 1;
	}
}

enum enum2 {
	aa = 0;
	bb = 1;
}

message TestMsg2 {
	int32 a = 1;
	int32 p = 2;
	string o = 4;
	message TestMsg3 {
		repeated string s = 2;
		enum enum3 {
			aaa = 0;
			bbb = 1;
		}
	}
	repeated int32 k = 3;
}

message TestMsg4 {
	int32 n = 1;
	int32 p = 2;
	string o = 4;
	message TestMsg5 {
		repeated string s = 2;
		message TestMsg6 {
			repeated string s = 2;
		}
	}
	repeated int32 k = 3;
	enum enum4 {
		aaaa = 0;
		bbbb = 1;
	}
}

enum enum5 {
	a = 0;
	b = 1;
}

message TestMsg7 {
	enum enum6 {
		a6 = 0;
		b6 = 1;
	}
	int32 n = 1;
	int32 p = 2;
	string o = 4;
	message TestMsg8 {
		repeated string s = 2;
		enum enum7 {
			a7 = 0;
			b7 = 1;
		}
	}
	repeated int32 k = 3;
}

message TestMsg9 {
	message TestMsg10 {
		string p = 2;
		enum enum8 {
			a8 = 0;
			b8 = 1;
		}
	}
}
)";

const char * MANY_FIELDS_PROTO = R"(
syntax = "proto3";

message TestMsg {
}

message TestMsg2 {
	int32 a = 10;
	int32 l = 2;
	string o = 4;

	int32 b = 1;
	int32 c = 3;
	string d = 5;
	message TestMsg3 {
		repeated string s = 2;
	}
	repeated int32 k = 6;
}

message TestMsg4 {
	int32 a = 10;
	int32 l = 2;
	string o = 4;
	message TestMsg5 {
		repeated string s = 2;
		message TestMsg6 {
			repeated string s = 2;
		}
	}
	repeated int32 k = 3;
}

message TestMsg5 {
	int32 a = 1;
	uint32 b = 2;
	string c = 3;
  bytes d = 4;
  fixed32 e = 5;
  sint64 f = 6;
	repeated bool g = 7;
}
)";

const char * MESSAGES_PROTO = R"(
syntax = "proto3";

message TestMsg {
	string name = 1;
}

message TestMsg2 {
	TestMsg msg = 1;
}
)";

const char * SCHEMA1_PROTO = R"(
syntax = "proto3";

message Schema1 {
	string name = 1;

  message Nested {
    message DoubleNested {
    }
  }
}
)";

const char * SCHEMA2_PROTO = R"(
syntax = "proto3";

message Schema2 {
	string name = 1;
  schema1.Schema1 schema1_dependency = 2;
  schema1.Schema1.Nested schema1_nested_dependency = 3;
  schema1.Schema1.Nested.DoubleNested schema1_double_nested_dependency = 4;

  message Nested {
    message DoubleNested {
    }
  }
}
)";

const char * SCHEMA3_PROTO = R"(
syntax = "proto3";

message Schema3 {
	string name = 1;
  repeated schema1.Schema1 schema1_dependency = 2;
  repeated schema1.Schema1.Nested schema1_nested_dependency = 3;
  repeated schema1.Schema1.Nested.DoubleNested schema1_double_nested_dependency = 4;
  repeated schema2.Schema2 schema2_dependency = 5;
  repeated schema2.Schema2.Nested schema2_nested_dependency = 6;
  repeated schema2.Schema2.Nested.DoubleNested schema2_double_nested_dependency = 7;
}
)";

const char * TEST_PROTO = R"(
syntax = "proto3";

message TestMsg {
	string name = 1;
	int32 number = 2;
	int32 num = 5;
	int32 opt = 3;
	repeated string array = 4;
	repeated int32 int_array = 7;
}
)";
