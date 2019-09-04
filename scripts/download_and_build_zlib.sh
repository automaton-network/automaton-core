ZLIB_VER="1.2.11"

# Download zlib
[ ! -d zlib ] && get_archive "https://zlib.net/zlib-$ZLIB_VER.tar.gz" \
  "zlib-1.2.11.tar.gz" "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"
[ -d zlib-$ZLIB_VER ] && mv zlib-$ZLIB_VER zlib

# Build zlib
if [ ! -f zlib/libz.a ]; then
  cd zlib
  ./configure --static && make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  zlib ALREADY BUILT"
  print_separator "=" 80
fi
