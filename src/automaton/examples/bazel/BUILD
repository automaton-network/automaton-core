genrule(
  name = "genrule",
  outs = ["genrule.out"],
  cmd = "echo hello world >$@",
)

# examples on how to use cmake in bazel

# genrule(
#   name = "cmake",
#   srcs = [
# 
#  ],
#   outs = ["build.log"],
#   cmd = "echo generating the cmake project >$@ \n" +
#         "cmake automaton/examples/bazel/cmake -G \"MinGW Makefiles\" -DCMAKE_SH=\"CMAKE_SH-NOTFOUND\" \n" +
#         "echo finished the generation of the cmake project >$@ \n" +
#         "echo building the cmake project with mingw32 >$@ \n" +
#         "mingw32-make \n" +
#         "echo finished build >$@ \n"
# )

# genrule(
#   name = "cmake_vs",
#   srcs = [
# 
#   ],
#   outs = ["build_vs.log"],
#   cmd = "echo generating the cmake project >$@ \n" +
#         "cmake automaton/examples/bazel/cmake \n" +
#         "echo finished the generation of the cmake project >$@ \n",
# )
