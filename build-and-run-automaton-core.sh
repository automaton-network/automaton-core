#!/bin/bash
CPUCOUNT=$(grep -c "^processor" /proc/cpuinfo)

cd src
mkdir -p logs
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$CPUCOUNT
cd ..
build/automaton-core-cli
# bazel build -c opt //automaton/core && bazel-bin/automaton/core/core # 2>/dev/null
#reset
