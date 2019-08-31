# OPENSSL_VER="1.1.1c" # f6fb3079ad15076154eda9413fed42877d668e7069d9b87396d0804fdb3f4c90
OPENSSL_VER="1.0.2s" # cabd5c9492825ce5bd23f3c3aeed6a97f8142f606d893df216411f07d1abab96

# Download OpenSSL
[ ! -d openssl ] && \
  get_archive "https://www.openssl.org/source/openssl-$OPENSSL_VER.tar.gz" \
  "openssl-$OPENSSL_VER.tar.gz" "cabd5c9492825ce5bd23f3c3aeed6a97f8142f606d893df216411f07d1abab96" && \
[ -d openssl-$OPENSSL_VER ] && mv openssl-$OPENSSL_VER openssl

# Build OpenSSL
if [ ! -f openssl/libcrypto.a ]; then
  cd openssl
  ./config no-shared
  make clean
  make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  OpenSSL ALREADY BUILT"
  print_separator "=" 80
fi
