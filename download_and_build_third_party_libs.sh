#!/bin/bash

darwin=false;
case "`uname`" in
  Darwin*) darwin=true ;;
esac

if $darwin; then
  sedi="sed -i ''"
else
  sedi="sed -i "
fi

print_separator() {
 str=$1
 num=$2
 v=$(printf "%-${num}s" "$str")
 echo "${v// /$str}"
}

LOCAL_3P="local_third_party"

cd src
mkdir -p $LOCAL_3P
cd $LOCAL_3P

function git_repo() {
  repo=$1
  dir=$2
  commit=$3

  print_separator "=" 80
  echo  Updating $dir from repo $repo
  print_separator "=" 80

  if [ ! -d $dir ]
  then
    git clone $repo $dir
  fi

  cd $dir

  if [ ! -z "$commit" ]
  then
    git reset --hard $commit
  else
    git pull
  fi

  cd ..
}

function get_archive() {
  url=$1
  filename=$2
  sha=$3

  print_separator "=" 80
  echo "  Downloading $filename"
  print_separator "=" 80

  [ ! -f $2 ] && wget $1

  filesha=$(shasum -a 256 $filename | cut -d' ' -f1)
  [ $filesha == $sha ] || ( echo "Error: Wrong hash [$filesha] Expected [$sha]" && exit 1 )

  echo "Extracting $filename"
  tar -xzf $filename
}

# Download all libraries
git_repo "https://github.com/LuaJIT/LuaJIT.git" "LuaJIT" "0bf80b07b0672ce874feedcc777afe1b791ccb5a"
git_repo "https://github.com/zeromq/libzmq.git" "libzmq" "d062edd8c142384792955796329baf1e5a3377cd"
git_repo "https://github.com/zeromq/cppzmq.git" "cppzmq" "d9f0f016c07046742738c65e1eb84722ae32d7d4"
git_repo "https://github.com/zeromq/zmqpp.git" "zmqpp" "f8ff127683dc555aa004c0e6e2b18d2354a375be"
git_repo "https://github.com/ThePhD/sol2.git" "sol2" "254466eb4b3ae630c731a557987f3adb1a8f86b0"
git_repo "https://github.com/AmokHuginnsson/replxx.git" "replxx" "3cb884e3fb4b1a28efeb716fac75f77eecc7ea3d"
git_repo "https://github.com/lua/lua.git" "lua" "e354c6355e7f48e087678ec49e340ca0696725b1"
git_repo "https://github.com/muflihun/easyloggingpp.git" "easyloggingpp" "a5317986d74b6dd3956021cb7fbb0669cce398b2"
git_repo "https://github.com/weidai11/cryptopp.git" "cryptopp" "c8d8caf70074655a2562ae1ea45cb30e28fee2b4"
git_repo "https://github.com/orlp/ed25519.git" "ed25519" "7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"
git_repo "https://github.com/google/googletest.git" "googletest" "2fe3bd994b3189899d93f1d5a881e725e046fdc2"
# git_repo "https://github.com/nlohmann/json.git" "json" "359f98d14065bf4e53eeb274f5987fd08f16e5bf"
git_repo "https://github.com/nelhage/rules_boost.git" "com_github_nelhage_rules_boost" "fe787183c14f2a5c6e5e1e75a7c57d2e799d3d19"
git_repo "https://github.com/protocolbuffers/protobuf.git" "protobuf" "48cb18e5c419ddd23d9badcfe4e9df7bde1979b2"
git_repo "https://github.com/svaarala/duktape.git" "duktape" "d7fdb67f18561a50e06bafd196c6b423af9ad6fe"

[ ! -d json-3.1.2 ] && \
  mkdir json-3.1.2 && \
  wget https://github.com/nlohmann/json/releases/download/v3.2.0/json.hpp -O json-3.1.2/json.hpp

[ ! -d lua-5.3.4 ] && \
  get_archive "https://github.com/lua/lua/releases/download/v5-3-4/lua-5.3.4.tar.gz" \
  "lua-5.3.4.tar.gz" "f681aa518233bc407e23acf0f5887c884f17436f000d453b2491a9f11a52400c"

[ ! -d boost_1_68_0 ] && \
  get_archive "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz" \
  "boost_1_68_0.tar.gz" "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf"

[ ! -d zlib-1.2.11 ] && get_archive "https://zlib.net/zlib-1.2.11.tar.gz" \
  "zlib-1.2.11.tar.gz" "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"

[ ! -d bzip2-1.0.6 ] && get_archive "https://fossies.org/linux/misc/bzip2-1.0.6.tar.gz" \
  "bzip2-1.0.6.tar.gz" "a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd"

[ ! -d xz-5.2.3 ] && \
  get_archive "http://phoenixnap.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz" \
  "xz-5.2.3.tar.gz" "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb"

# Build LuaJIT
print_separator "=" 80
echo "  BUILDING LuaJIT"
print_separator "=" 80

# Create hpp file
cd lua
cat > lua.hpp << EOL
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
EOL
cd ..

cd LuaJIT
make
cd ..

# Build libzmq
print_separator "=" 80
echo "   BUILDING libzmq"
print_separator "=" 80

unset GREP_COLOR
unset GREP_OPTIONS
cd libzmq
[ ! -f configure ] && ./autogen.sh && ./configure
make
cd ..

# Build zmqpp
print_separator "=" 80
echo "  BUILDING zmqpp"
print_separator "=" 80

cd zmqpp
$sedi 's/CUSTOM_INCLUDE_PATH =/CUSTOM_INCLUDE_PATH = -I..\/libzmq\/include/' Makefile
$sedi 's/LIBRARY_LIBS =/LIBRARY_LIBS = -L..\/libzmq\/src\/.libs/' Makefile
make
cd ..

# Build replxx
print_separator "=" 80
echo "  BUILDING replxx"
print_separator "=" 80

cd replxx
mkdir -p build && cd build
[ ! -f CMakeCache.txt ] && cmake -DCMAKE_BUILD_TYPE=Release ..
make replxx
cd ../..

# Build boost
print_separator "=" 80
echo "  BUILDING boost"
print_separator "=" 80

cd boost_1_68_0
[ ! -f b2 ] && ./bootstrap.sh
[ ! -d stage ] && ./b2 \
  --with-filesystem --with-system --with-iostreams \
  cxxstd=14 link=static runtime-link=static stage
cd ..
