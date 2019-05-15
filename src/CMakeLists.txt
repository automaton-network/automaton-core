cmake_minimum_required(VERSION 3.0)
project(automaton-network)

set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "1")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_PACKAGE_VENDOR "Automaton Network")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/lua
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/replxx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/json-3.1.2
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/cryptopp
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/ed25519/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/build/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/sol2/single/sol
)

file(
  GLOB CRYPTO_FILES
  automaton/core/cli/*.cc
  automaton/core/common/*.cc
  automaton/core/crypto/*.cc
  automaton/core/crypto/cryptopp/*.cc
  automaton/core/crypto/ed25519_orlp/*.cc
  automaton/core/data/*.cc
  automaton/core/data/protobuf/*.cc
  automaton/core/io/*.cc
  automaton/core/network/*.cc
  automaton/core/node/*.cc
  automaton/core/node/lua_node/*.cc
  automaton/core/script/bind_*.cc
  automaton/core/script/engine.cc
  automaton/core/smartproto/*.cc
  automaton/core/state/state*.cc
  automaton/core/storage/*.cc
  automaton/core/testnet/*.cc
)

add_library(automaton-core STATIC ${CRYPTO_FILES})

add_executable(
  automaton-core-cli
  automaton/core/core.cc
)
set (CMAKE_CXX_FLAGS "-Wl,--no-as-needed")

target_link_libraries(
  automaton-core-cli
  automaton-core
  Threads::Threads
  ${CMAKE_DL_LIBS}

  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_system.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/cryptopp/libcryptopp.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/build/libg3logger.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/lua/liblua.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src/.libs/libprotobuf-lite.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src/.libs/libprotobuf.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/replxx/build/libreplxx.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_filesystem.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_iostreams.a
)
