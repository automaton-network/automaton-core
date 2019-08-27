PROTOBUF_VER="3.6.1.2"

# Download protobuf
[ ! -d protobuf ] && \
  get_archive "https://github.com/protocolbuffers/protobuf/archive/v$PROTOBUF_VER.tar.gz" \
  "v$PROTOBUF_VER.tar.gz" "2244b0308846bb22b4ff0bcc675e99290ff9f1115553ae9671eba1030af31bc0"
[ -d protobuf-$PROTOBUF_VER ] && mv protobuf-$PROTOBUF_VER protobuf

# Build protobuf
if [ ! -f protobuf/src/.libs/libprotobuf.a ]; then
  print_separator "=" 80
  echo "  BUILDING protobuf"
  print_separator "=" 80

  cd protobuf
  [ ! -f configure ] && ./autogen.sh
  [ ! -f Makefile ] && ./configure
  make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  protobuf ALREADY BUILT"
  print_separator "=" 80
fi
