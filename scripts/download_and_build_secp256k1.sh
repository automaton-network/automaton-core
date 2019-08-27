BITCOIN_VER="0.17.1"

# Downlaod bitcoin (contains libsecp256k1)
[ ! -d bitcoin ] && \
  get_archive "https://github.com/bitcoin/bitcoin/archive/v$BITCOIN_VER.tar.gz" \
  "v$BITCOIN_VER.tar.gz" "d51bae80fc0a460ce752d04097c4a1271a66b55260d53165d82313488117d290"
[ -d bitcoin-$BITCOIN_VER ] && mv bitcoin-$BITCOIN_VER bitcoin

# Build libsecp256k1
if [ ! -f bitcoin/src/secp256k1/.libs/libsecp256k1.a ]; then
  print_separator "=" 80
  echo "  BUILDING libsecp256k1"
  print_separator "=" 80

  cd bitcoin/src/secp256k1
  ./autogen.sh
  ./configure --enable-module-recovery
  make -j$CPUCOUNT
  ./tests
  cd ../../..
else
  print_separator "=" 80
  echo "  libsecp256k1 ALREADY BUILT"
  print_separator "=" 80
fi
