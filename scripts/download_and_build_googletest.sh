GTEST_VER="1.8.1"

# Downlaod GTest
[ ! -d googletest ] && \
  get_archive "https://github.com/google/googletest/archive/release-$GTEST_VER.tar.gz" \
  "googletest-release-$GTEST_VER.tar.gz" "9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c"
[ -d googletest-release-$GTEST_VER ] && mv googletest-release-$GTEST_VER googletest

# Build GTest
if [ ! -f googletest/build/googlemock/gtest/libgtest_main.a ]; then
  print_separator "=" 80
  echo "  BUILDING GTest"
  print_separator "=" 80

  cd googletest
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$CPUCOUNT
  cd ../..
else
  print_separator "=" 80
  echo "  GTest ALREADY BUILT"
  print_separator "=" 80
fi
