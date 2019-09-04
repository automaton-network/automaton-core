PCRE_VER="8.43"

# Download pcre
[ ! -d pcre ] && get_archive "https://ftp.pcre.org/pub/pcre/pcre-$PCRE_VER.tar.gz" \
  "pcre-$PCRE_VER.tar.gz" "0b8e7465dc5e98c757cc3650a20a7843ee4c3edf50aaf60bb33fd879690d2c73"
[ -d pcre-$PCRE_VER ] && mv pcre-$PCRE_VER pcre

if [ ! -d pcre ]; then
  print_separator "=" 80
  echo "  pcre NOT DOWNLOADED!"
  print_separator "=" 80
  exit
fi

# Build pcre
if [ ! -f pcre/.libs/libpcre.a ]; then
  print_separator "=" 80
  echo "  BUILDING pcre"
  print_separator "=" 80
  cd pcre
  ./configure && make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  pcre ALREADY BUILT"
  print_separator "=" 80
fi
