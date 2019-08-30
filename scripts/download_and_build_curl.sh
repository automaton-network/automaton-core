CURL_VER="7.65.3"

# Download curl
[ ! -d curl ] && \
  get_archive "https://github.com/curl/curl/releases/download/curl-${CURL_VER//\./_}/curl-$CURL_VER.tar.gz" \
  "curl-$CURL_VER.tar.gz" "4376ac72b95572fb6c4fbffefb97c7ea0dd083e1974c0e44cd7e49396f454839"
[ -d curl-$CURL_VER ] && mv curl-$CURL_VER curl

# Build curl
if [ ! -f curl/lib/.libs/libcurl.a ]; then
  print_separator "=" 80
  echo "  BUILDING curl"
  print_separator "=" 80

  cd curl
  ./configure --disable-shared && make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  curl ALREADY BUILT"
  print_separator "=" 80
fi
