#!/bin/bash

unset GREP_COLOR
unset GREP_OPTIONS

# Get the root directory where our script lives.
AUTOMATON_ROOT="$( cd "$(dirname "$0")" ; pwd -P )"

. scripts/utils.sh

LOCAL_3P="local_third_party"
cd src
mkdir -p $LOCAL_3P
cd $LOCAL_3P

# Download all libraries
# git_repo "https://github.com/LuaJIT/LuaJIT.git" "LuaJIT" "0bf80b07b0672ce874feedcc777afe1b791ccb5a"
# git_repo "https://github.com/zeromq/libzmq.git" "libzmq" "d062edd8c142384792955796329baf1e5a3377cd"
# git_repo "https://github.com/zeromq/cppzmq.git" "cppzmq" "d9f0f016c07046742738c65e1eb84722ae32d7d4"
# git_repo "https://github.com/zeromq/zmqpp.git" "zmqpp" "f8ff127683dc555aa004c0e6e2b18d2354a375be"
# git_repo "https://github.com/ThePhD/sol2.git" "sol2" "254466eb4b3ae630c731a557987f3adb1a8f86b0"
# git_repo "https://github.com/AmokHuginnsson/replxx.git" "replxx" "3cb884e3fb4b1a28efeb716fac75f77eecc7ea3d"
# git_repo "https://github.com/lua/lua.git" "lua" "e354c6355e7f48e087678ec49e340ca0696725b1"
# git_repo "https://github.com/weidai11/cryptopp.git" "cryptopp" "c8d8caf70074655a2562ae1ea45cb30e28fee2b4"
# git_repo "https://github.com/orlp/ed25519.git" "ed25519" "7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"
# git_repo "https://github.com/google/googletest.git" "googletest" "2fe3bd994b3189899d93f1d5a881e725e046fdc2"
# git_repo "https://github.com/nlohmann/json.git" "json" "359f98d14065bf4e53eeb274f5987fd08f16e5bf"
# git_repo "https://github.com/nelhage/rules_boost.git" "com_github_nelhage_rules_boost" "fe787183c14f2a5c6e5e1e75a7c57d2e799d3d19"
# git_repo "https://github.com/protocolbuffers/protobuf.git" "protobuf" "48cb18e5c419ddd23d9badcfe4e9df7bde1979b2"
# git_repo "https://github.com/svaarala/duktape.git" "duktape" "d7fdb67f18561a50e06bafd196c6b423af9ad6fe"

JSON_VER="3.2.0"
[ ! -d json ] && \
  mkdir json && \
  curl -L https://github.com/nlohmann/json/releases/download/v$JSON_VER/json.hpp -o json/json.hpp

[ ! -f sol2/single/sol/sol.hpp ] && \
  mkdir -p sol2/single/sol && \
  curl -L https://github.com/ThePhD/sol2/releases/download/v2.20.6/sol.hpp -o sol2/single/sol/sol.hpp

#[ ! -d bzip2-1.0.6 ] && get_archive "http://anduin.linuxfromscratch.org/LFS/bzip2-1.0.6.tar.gz" \
#  "bzip2-1.0.6.tar.gz" "a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd"

#[ ! -d xz-5.2.3 ] && \
#  get_archive "http://phoenixnap.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz" \
#  "xz-5.2.3.tar.gz" "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb"

# OpenSSL is used by libcurl so needs to be built prior.
. $AUTOMATON_ROOT/scripts/download_and_build_openssl.sh

. $AUTOMATON_ROOT/scripts/download_and_build_boost.sh
. $AUTOMATON_ROOT/scripts/download_and_build_cryptopp.sh
. $AUTOMATON_ROOT/scripts/download_and_build_curl.sh
. $AUTOMATON_ROOT/scripts/download_and_build_ed25519.sh
. $AUTOMATON_ROOT/scripts/download_and_build_g3log.sh
. $AUTOMATON_ROOT/scripts/download_and_build_gmp.sh
. $AUTOMATON_ROOT/scripts/download_and_build_googletest.sh
. $AUTOMATON_ROOT/scripts/download_and_build_JUCE.sh
. $AUTOMATON_ROOT/scripts/download_and_build_lua.sh
. $AUTOMATON_ROOT/scripts/download_and_build_LuaJIT.sh
. $AUTOMATON_ROOT/scripts/download_and_build_protobuf.sh
. $AUTOMATON_ROOT/scripts/download_and_build_replxx.sh
. $AUTOMATON_ROOT/scripts/download_and_build_secp256k1.sh
. $AUTOMATON_ROOT/scripts/download_and_build_zlib.sh
