cmake_minimum_required(VERSION 3.0)
project(AUTOMATON_CORE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/lua
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
