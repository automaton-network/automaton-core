#!/bin/bash


PROTOBUF_VER="3.6.1.2"
JUCE_VER="5.4.3"
CRYPTOPP_VER="7_0_0"
LUA_VER="5.3.5"
G3LOG_VER="1.3.2"
GMPVER="6.1.2"

darwin=false;
case "`uname`" in
  Darwin*) darwin=true ;;
esac

if $darwin; then
  sedi="sed -i ''"
  CPUCOUNT=$(sysctl -n hw.ncpu)
else
  sedi="sed -i "
  CPUCOUNT=$(grep -c "^processor" /proc/cpuinfo)
fi

echo "$CPUCOUNT logical cores"

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
  echo "  Downloading $filename from $url"
  print_separator "=" 80

  [ ! -f $2 ] && curl -L $1 -o $2
  filesha=$(shasum -a 256 $filename | cut -d' ' -f1)
  if [ $filesha != $sha ]; then
    echo "Error: Wrong hash [$filesha] Expected [$sha]"
    exit 1
  else
    print_separator "=" 80
    echo "  Extracting $filename"
    print_separator "=" 80

    tar -xf $filename
  fi;
}

# Download all libraries
# git_repo "https://github.com/LuaJIT/LuaJIT.git" "LuaJIT" "0bf80b07b0672ce874feedcc777afe1b791ccb5a"
# git_repo "https://github.com/zeromq/libzmq.git" "libzmq" "d062edd8c142384792955796329baf1e5a3377cd"
# git_repo "https://github.com/zeromq/cppzmq.git" "cppzmq" "d9f0f016c07046742738c65e1eb84722ae32d7d4"
# git_repo "https://github.com/zeromq/zmqpp.git" "zmqpp" "f8ff127683dc555aa004c0e6e2b18d2354a375be"
# git_repo "https://github.com/ThePhD/sol2.git" "sol2" "254466eb4b3ae630c731a557987f3adb1a8f86b0"
# git_repo "https://github.com/AmokHuginnsson/replxx.git" "replxx" "3cb884e3fb4b1a28efeb716fac75f77eecc7ea3d"
# git_repo "https://github.com/lua/lua.git" "lua" "e354c6355e7f48e087678ec49e340ca0696725b1"
# git_repo "https://github.com/weidai11/cryptopp.git" "cryptopp" "c8d8caf70074655a2562ae1ea45cb30e28fee2b4"
git_repo "https://github.com/orlp/ed25519.git" "ed25519" "7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"
# git_repo "https://github.com/google/googletest.git" "googletest" "2fe3bd994b3189899d93f1d5a881e725e046fdc2"
# git_repo "https://github.com/nlohmann/json.git" "json" "359f98d14065bf4e53eeb274f5987fd08f16e5bf"
# git_repo "https://github.com/nelhage/rules_boost.git" "com_github_nelhage_rules_boost" "fe787183c14f2a5c6e5e1e75a7c57d2e799d3d19"
# git_repo "https://github.com/protocolbuffers/protobuf.git" "protobuf" "48cb18e5c419ddd23d9badcfe4e9df7bde1979b2"
# git_repo "https://github.com/svaarala/duktape.git" "duktape" "d7fdb67f18561a50e06bafd196c6b423af9ad6fe"

[ ! -d gmp ] && \
  get_archive "https://gmplib.org/download/gmp/gmp-$GMPVER.tar.xz" \
  "gmp-$GMPVER.tar.xz" "87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912"
[ -d "gmp-$GMPVER" ] && mv "gmp-$GMPVER" gmp

[ ! -d g3log ] && \
  get_archive "https://github.com/KjellKod/g3log/archive/$G3LOG_VER.tar.gz" \
  "g3log-$G3LOG_VER.tar.gz" "0ed1983654fdd8268e051274904128709c3d9df8234acf7916e9015199b0b247"
[ -d g3log-$G3LOG_VER ] && mv g3log-$G3LOG_VER g3log

[ ! -d replxx ] && \
  get_archive "https://github.com/AmokHuginnsson/replxx/archive/release-0.0.1.tar.gz" \
  "replxx-release-0.0.1.tar.gz" "af0576e401e43d88fadabdc193e7cbed20d0a8538ae3d9228732211d1b255348"
[ -d replxx-release-0.0.1 ] && mv replxx-release-0.0.1 replxx

[ ! -d lua ] && \
  get_archive "https://www.lua.org/ftp/lua-$LUA_VER.tar.gz" \
  "lua-$LUA_VER.tar.gz" "0c2eed3f960446e1a3e4b9a1ca2f3ff893b6ce41942cf54d5dd59ab4b3b058ac"
[ -d lua-$LUA_VER ] && mv lua-$LUA_VER/src lua && rm -rf lua-$LUA_VER

[ ! -d LuaJIT ] && \
  get_archive "https://github.com/LuaJIT/LuaJIT/archive/v2.0.5.tar.gz" \
  "LuaJIT-2.0.5.tar.gz" "8bb29d84f06eb23c7ea4aa4794dbb248ede9fcb23b6989cbef81dc79352afc97"
[ -d LuaJIT-2.0.5 ] && mv LuaJIT-2.0.5 LuaJIT

[ ! -d googletest ] && \
  get_archive "https://github.com/google/googletest/archive/release-1.8.1.tar.gz" \
  "googletest-release-1.8.1.tar.gz" "9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c"
[ -d googletest-release-1.8.1 ] && mv googletest-release-1.8.1 googletest

[ ! -d cryptopp ] && \
  get_archive "https://github.com/weidai11/cryptopp/archive/CRYPTOPP_$CRYPTOPP_VER.tar.gz" \
  "CRYPTOPP_$CRYPTOPP_VER.tar.gz" "3ee97903882b5f58c88b6f9d2ce50fd1000be95479180c7b4681cd3f4c1c7629" && \
[ -d cryptopp-CRYPTOPP_$CRYPTOPP_VER ] && mv cryptopp-CRYPTOPP_$CRYPTOPP_VER cryptopp

[ ! -d protobuf ] && \
  get_archive "https://github.com/protocolbuffers/protobuf/archive/v$PROTOBUF_VER.tar.gz" \
  "v$PROTOBUF_VER.tar.gz" "2244b0308846bb22b4ff0bcc675e99290ff9f1115553ae9671eba1030af31bc0"
[ -d protobuf-$PROTOBUF_VER ] && mv protobuf-$PROTOBUF_VER protobuf

[ ! -d json-3.1.2 ] && \
  mkdir json-3.1.2 && \
  curl -L https://github.com/nlohmann/json/releases/download/v3.2.0/json.hpp -o json-3.1.2/json.hpp

[ ! -f sol2/single/sol/sol.hpp ] && \
  mkdir -p sol2/single/sol && \
  curl -L https://github.com/ThePhD/sol2/releases/download/v2.20.6/sol.hpp -o sol2/single/sol/sol.hpp

[ ! -d boost ] && \
  get_archive "https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz" \
  "boost_1_70_0.tar.gz" "882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9"
[ -d boost_1_70_0 ] && mv boost_1_70_0 boost

[ ! -d zlib-1.2.11 ] && get_archive "https://zlib.net/zlib-1.2.11.tar.gz" \
  "zlib-1.2.11.tar.gz" "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"

[ ! -d bzip2-1.0.6 ] && get_archive "https://fossies.org/linux/misc/bzip2-1.0.6.tar.gz" \
  "bzip2-1.0.6.tar.gz" "a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd"

[ ! -d xz-5.2.3 ] && \
  get_archive "http://phoenixnap.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz" \
  "xz-5.2.3.tar.gz" "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb"

[ ! -d JUCE ] && \
  get_archive "https://github.com/WeAreROLI/JUCE/archive/$JUCE_VER.tar.gz" \
  "JUCE-$JUCE_VER.tar.gz" "05cfec616c854d0f8f6646c0d8a7ac868410b25a3ba2c839879a7904504d5403"
[ -d JUCE-$JUCE_VER ] && mv JUCE-$JUCE_VER JUCE

[ ! -d bitcoin ] && \
  get_archive "https://github.com/bitcoin/bitcoin/archive/v0.17.1.tar.gz" \
  "v0.17.1.tar.gz" "d51bae80fc0a460ce752d04097c4a1271a66b55260d53165d82313488117d290"
[ -d bitcoin-0.17.1 ] && mv bitcoin-0.17.1 bitcoin

# Build Lua
if [ ! -f lua/liblua.a ]; then
  print_separator "=" 80
  echo "  BUILDING Lua"
  print_separator "=" 80

  cd lua
  make -j$CPUCOUNT linux a
  cd ..
else
  print_separator "=" 80
  echo "  Lua ALREADY BUILT"
  print_separator "=" 80
fi

# Build LuaJIT
if [ ! -f LuaJIT/src/libluajit.a ]; then
  print_separator "=" 80
  echo "  BUILDING LuaJIT"
  print_separator "=" 80

  cd LuaJIT
  if $darwin; then
    make -j$CPUCOUNT MACOSX_DEPLOYMENT_TARGET=`sw_vers -productVersion`
  else
    make -j$CPUCOUNT
  fi
  cd ..
else
  print_separator "=" 80
  echo "  LuaJIT ALREADY BUILT"
  print_separator "=" 80
fi

unset GREP_COLOR
unset GREP_OPTIONS

# Build GTest
if [ ! -f googletest/build/googlemock/gtest/libgtest_main.a ]; then
  print_separator "=" 80
  echo "  BUILDING GTest"
  print_separator "=" 80

  cd googletest
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$CPUCOUNT
  cd ../..
else
  print_separator "=" 80
  echo "  GTest ALREADY BUILT"
  print_separator "=" 80
fi

# Build JUCE Projucer -- only when not in CI
if [ ! -f JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer ]; then
  if [ ! $CI ]; then
    print_separator "=" 80
    echo "  BUILDING Projucer"
    print_separator "=" 80

    if $darwin; then
      # TODO(asen): Need to do build for Mac OS
      pass;
    else
      cd JUCE/extras/Projucer/Builds/LinuxMakefile
      make -j$CPUCOUNT CPPFLAGS="-DJUCER_ENABLE_GPL_MODE=1" CONFIG=Release # V=1 for verbose
      cd ../../../../..
    fi
  fi
else
  print_separator "=" 80
  echo "  Projucer ALREADY BUILT"
  print_separator "=" 80
fi

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

# Build replxx
if [ ! -f replxx/build/libreplxx.a ]; then
  print_separator "=" 80
  echo "  BUILDING replxx"
  print_separator "=" 80

  cd replxx
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$CPUCOUNT replxx
  cd ../..
else
  print_separator "=" 80
  echo "  replxx ALREADY BUILT"
  print_separator "=" 80
fi

# Build g3log
if [ ! -f g3log/build/libg3logger.a ]; then
  print_separator "=" 80
  echo "  BUILDING g3log"
  print_separator "=" 80

  cd g3log
  mkdir -p build && cd build
  [ ! -f CMakeCache.txt ] && cmake .. -DCHANGE_G3LOG_DEBUG_TO_DBUG=ON -DG3_SHARED_LIB=OFF -DCMAKE_BUILD_TYPE=Release
  make -j$CPUCOUNT
  cd ../..
else
  print_separator "=" 80
  echo "  g3log ALREADY BUILT"
  print_separator "=" 80
fi

if [ ! -f gmp/.libs/libgmp.a ]; then
  print_separator "=" 80
  echo "  BUILDING gmp"
  print_separator "=" 80

  cd gmp
  ./configure
  make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  gmp ALREADY BUILT"
  print_separator "=" 80
fi

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
