G3LOG_VER="1.3.2"

# Download g3log
[ ! -d g3log ] && \
  get_archive "https://github.com/KjellKod/g3log/archive/$G3LOG_VER.tar.gz" \
  "g3log-$G3LOG_VER.tar.gz" "0ed1983654fdd8268e051274904128709c3d9df8234acf7916e9015199b0b247"
[ -d g3log-$G3LOG_VER ] && mv g3log-$G3LOG_VER g3log

# Build g3log
if [ ! -f g3log/build/libg3logger.a ]; then
  print_separator "=" 80
  echo "  BUILDING g3log"
  print_separator "=" 80

  cd g3log
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCHANGE_G3LOG_DEBUG_TO_DBUG=ON -DG3_SHARED_LIB=OFF -DCMAKE_BUILD_TYPE=Release -DUSE_DYNAMIC_LOGGING_LEVELS=ON
  make -j$CPUCOUNT
  cd ../..
else
  print_separator "=" 80
  echo "  g3log ALREADY BUILT"
  print_separator "=" 80
fi
