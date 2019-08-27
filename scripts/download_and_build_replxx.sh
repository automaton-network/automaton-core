REPLXX_VER="0.0.1"

# Downlaod replxx
[ ! -d replxx ] && \
  get_archive "https://github.com/AmokHuginnsson/replxx/archive/release-$REPLXX_VER.tar.gz" \
  "replxx-release-$REPLXX_VER.tar.gz" "af0576e401e43d88fadabdc193e7cbed20d0a8538ae3d9228732211d1b255348"
[ -d replxx-release-$REPLXX_VER ] && mv replxx-release-$REPLXX_VER replxx

# Build replxx
if [ ! -f replxx/build/libreplxx.a ]; then
  print_separator "=" 80
  echo "  BUILDING replxx"
  print_separator "=" 80

  cd replxx
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$CPUCOUNT replxx
  cd ../..
else
  print_separator "=" 80
  echo "  replxx ALREADY BUILT"
  print_separator "=" 80
fi
