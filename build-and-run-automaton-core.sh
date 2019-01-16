#/bin/bash
cd src
mkdir -p logs
bazel build --incompatible_package_name_is_a_function=false -c opt //automaton/core && bazel-bin/automaton/core/core # 2>/dev/null
#reset
