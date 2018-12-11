cd src
New-Item -ItemType Directory -Path .\logs
bazel build -c opt //automaton/core
bazel-bin/automaton/core/core
cd ..