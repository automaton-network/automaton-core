CRYPTOPP_VER="7_0_0"

# Download crypto++

[ ! -d cryptopp ] && \
  get_archive "https://github.com/weidai11/cryptopp/archive/CRYPTOPP_$CRYPTOPP_VER.tar.gz" \
  "CRYPTOPP_$CRYPTOPP_VER.tar.gz" "3ee97903882b5f58c88b6f9d2ce50fd1000be95479180c7b4681cd3f4c1c7629" && \
[ -d cryptopp-CRYPTOPP_$CRYPTOPP_VER ] && mv cryptopp-CRYPTOPP_$CRYPTOPP_VER cryptopp

# Build crypto++
if [ ! -f cryptopp/libcryptopp.a ]; then
  print_separator "=" 80
  echo "  BUILDING crypto++"
  print_separator "=" 80

  cd cryptopp
  make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  crypto++ ALREADY BUILT"
  print_separator "=" 80
fi
