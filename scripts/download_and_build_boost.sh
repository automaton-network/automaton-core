BOOST_VER="1.70.0"

# Download boost
[ ! -d boost ] && \
  get_archive "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_${BOOST_VER//\./_}.tar.gz" \
  "boost_${BOOST_VER//\./_}.tar.gz" "882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9"
[ -d boost_${BOOST_VER//\./_} ] && mv boost_${BOOST_VER//\./_} boost

# Build boost
if [ ! -f boost/stage/lib/libboost_filesystem.a ]; then
  print_separator "=" 80
  echo "  BUILDING boost"
  print_separator "=" 80

  cd boost
  [ ! -f b2 ] && ./bootstrap.sh
  [ ! -d stage ] && ./b2 \
    --with-filesystem --with-system --with-iostreams \
    cxxstd=14 link=static runtime-link=static stage
  cd ..
else
  print_separator "=" 80
  echo "  boost ALREADY BUILT"
  print_separator "=" 80
fi
