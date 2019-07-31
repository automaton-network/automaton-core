cmake_minimum_required(VERSION 3.0)
project(automaton-network)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VENDOR "Automaton Network")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  # This is needed, otherwise linking with libdl fails on Linux
  set(CMAKE_CXX_FLAGS "-Wl,--no-as-needed")
endif()

set(LOCAL_THIRD_PARTY_LIBS
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/bitcoin/src/secp256k1/.libs/libsecp256k1.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_filesystem.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_iostreams.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost/stage/lib/libboost_system.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/cryptopp/libcryptopp.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/build/libg3logger.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/gmp/.libs/libgmp.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/lua/liblua.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src/.libs/libprotobuf-lite.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src/.libs/libprotobuf-lite.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src/.libs/libprotobuf.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/replxx/build/libreplxx.a
)

set(GTEST_LIBS
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/build/googlemock/gtest/libgtest_main.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/build/googlemock/gtest/libgtest.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/build/googlemock/libgmock_main.a
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/build/googlemock/libgmock.a
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/boost
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/cryptopp
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/ed25519/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/build/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/g3log/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/gmp
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/googlemock/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/googletest/googletest/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/json-3.1.2
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/lua
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/protobuf/src
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/replxx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/sol2/single/sol
  ${CMAKE_CURRENT_SOURCE_DIR}/local_third_party/bitcoin/src
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
  automaton/tools/miner/miner.cc
  local_third_party/ed25519/src/*.c
)

add_library(automaton-core STATIC ${CRYPTO_FILES})

target_link_libraries(
  automaton-core
  Threads::Threads
  ${CMAKE_DL_LIBS}
  ${LOCAL_THIRD_PARTY_LIBS}
)

add_executable(automaton-core-cli automaton/core/core.cc)
target_link_libraries(automaton-core-cli automaton-core)

# === TOOLS ===

add_executable(
  automaton-miner
  automaton/tools/miner/miner_main.cc
  automaton/tools/miner/miner.cc
)
target_link_libraries(automaton-miner automaton-core)

enable_testing()
SET(CTEST_OUTPUT_ON_FAILURE ON)

macro(automaton_test test_path test_name)
  add_executable(
    ${test_name}
    automaton/tests/${test_path}/${test_name}.cc
    automaton/tests/data/proto_files.cc
  )
  target_link_libraries(${test_name} automaton-core ${GTEST_LIBS})
  add_test(
    NAME ${test_name}
    COMMAND ${test_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endmacro()

automaton_test(crypto test_ed25519_orlp)
automaton_test(crypto test_hash_transformation)
automaton_test(crypto test_Keccak_256_cryptopp)
automaton_test(crypto test_RIPEMD160_cryptopp)
automaton_test(crypto test_secp256k1_cryptopp)
automaton_test(crypto test_SHA256_cryptopp)
automaton_test(crypto test_SHA3_256_cryptopp)
automaton_test(crypto test_SHA512_cryptopp)

automaton_test(data protobuf_schema_all_data_types)
automaton_test(data protobuf_schema_gtest)
automaton_test(data protobuf_schema_test_empty_schema)
automaton_test(data protobuf_schema_test_enums)
automaton_test(data protobuf_schema_test_find_all_enums)
automaton_test(data protobuf_schema_test_find_all_fields)
automaton_test(data protobuf_schema_test_find_enum)
automaton_test(data protobuf_schema_test_find_existing_schema)
automaton_test(data protobuf_schema_test_find_unexisting_schema)
automaton_test(data protobuf_schema_test_invalid_data)
automaton_test(data protobuf_schema_test_message_serialization)
automaton_test(data protobuf_schema_test_setting_fields)

automaton_test(io test_io)

automaton_test(miner miner_test)

# automaton_test(network rpc_server_test)
# automaton_test(network http_server_test)

automaton_test(script test_script)

automaton_test(state state_test)
automaton_test(state state_persistent_test)

automaton_test(storage blobstore_test)
automaton_test(storage persistent_blobstore_test)
# automaton_test(storage persistent_storage_test)
automaton_test(storage persistent_vector_test)

add_library(
  blockchain_cpp_node
  automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node.cc
)

target_link_libraries(
  blockchain_cpp_node
  automaton-core
)

add_executable(
  demo_100_nodes
  automaton/examples/node/blockchain_cpp_node/blockchain_cpp_node_test_one_mining_thread.cc
)

target_link_libraries(
  demo_100_nodes
  blockchain_cpp_node
  automaton-core
)

add_library(
  koh-miner
  automaton/tools/miner/miner.cc
)

target_link_libraries(
  koh-miner
  automaton-core
)

add_executable(
  koh-miner-bin
  automaton/tools/miner/miner_main.cc
)

target_link_libraries(
  koh-miner-bin
  automaton-core
  koh-miner
)

add_library(
  eth_contract
  automaton/core/eth_contract/eth_contract.cc
)

target_link_libraries(
  eth_contract
  automaton-core
)

add_executable(
  eth_contract_test
  automaton/core/eth_contract/eth_contract_test.cc
)

target_link_libraries(
  eth_contract_test
  eth_contract
)
