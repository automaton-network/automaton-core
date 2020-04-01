cmake_minimum_required(VERSION 3.1)

if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW) # MSVC_RUNTIME_VERSION support
endif()

project(automaton-network)

option(automaton_STATIC_RUNTIME "Use static C runtime" ON)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  if (automaton_STATIC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
  option(automaton_USE_OPENSSL "Use OpenSSL" ON)
else()
  set(automaton_USE_OPENSSL ON)
endif()

option(automaton_BUILD_DEPENDENCIES "Build third-party libraries" ON)

option(automaton_RUN_GANACHE_TESTS "Run tests with local ganache server" OFF)


if (MSVC)
  add_compile_options(
    /W4
    /wd4100
#    /wd4101
    /wd4127
#    /wd4244
#    /wd4245
#    /wd4458
#    /wd4505
#    /wd4702
#    /wd4996
    /WX
  )
else()
  add_compile_options(
    -Wall
    -Wextra
    -Wshadow
#    -Wduplicated-cond
#    -Wduplicated-branches
#    -Wlogical-op
    -Wnull-dereference
#    -Wold-style-cast
#    -Wuseless-cast
#    -Wjump-misses-init
    -Wformat=2
    -Wno-unused-parameter
    -Wno-uninitialized
#    -Werror
  )
endif()


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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(
  GLOB CRYPTO_FILES
  automaton/core/cli/*.cc
  automaton/core/common/*.cc
  automaton/core/crypto/*.cc
  automaton/core/crypto/cryptopp/*.cc
  automaton/core/crypto/ed25519_orlp/*.cc
  automaton/core/data/*.cc
  automaton/core/data/protobuf/*.cc
  automaton/core/interop/ethereum/eth_contract_curl.cc
  automaton/core/interop/ethereum/eth_transaction.cc
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
)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_library(automaton-core STATIC ${CRYPTO_FILES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(CRYPTOPP_LIB cryptopp-static)
  set(PROTOBUF_LIB libprotobuf)
  set(CURL_LIB libcurl)
  set(BOOST_LIBS libboost_system;libboost_date_time;libboost_iostreams;libboost_filesystem)
  if (automaton_USE_OPENSSL)
    set(OPENSSL_LIBS libssl;libcrypto)
  else()
    set(OPENSSL_LIBS "")
  endif()
  if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(GTEST_LIBS gtestd;gtest_maind)
    set(REPLXX_LIB replxx-d)
    set(PROTOBUF_LIB libprotobufd)
    set(CURL_LIB libcurl-d)
    set(Z_LIB zlibstaticd)
  else()
    set(GTEST_LIBS gtest;gtest_main)
    set(REPLXX_LIB replxx)
    set(PROTOBUF_LIB libprotobuf)
    set(CURL_LIB libcurl)
    set(Z_LIB zlibstatic)
  endif()
else()
  set(CRYPTOPP_LIB cryptopp)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(PROTOBUF_LIB protobufd)
    set(GTEST_LIBS gtestd;gtest_maind)
    set(CURL_LIB curl-d)
  else()
    set(PROTOBUF_LIB protobuf)
    set(GTEST_LIBS gtest;gtest_main)
    set(CURL_LIB curl)
  endif()
  set(BOOST_LIBS boost_system;boost_date_time;boost_iostreams;boost_filesystem)
  set(REPLXX_LIB replxx)
  set(OPENSSL_LIBS ssl;crypto)
  set(Z_LIB z)
  set(GMP_LIB ${CMAKE_INSTALL_PREFIX}/lib/libgmp.a)
  if (automaton_BUILD_DEPENDENCIES)
    add_dependencies(automaton-core ext_gmp ext_secp256k1)
  endif()
endif()

target_link_libraries(
  automaton-core
  Threads::Threads
  ${REPLXX_LIB}
  ${CRYPTOPP_LIB}
  secp256k1
  g3logger
  ed25519
  ${PROTOBUF_LIB}
  ${CURL_LIB}
  lua
  ${BOOST_LIBS}
  ${OPENSSL_LIBS}
  ${Z_LIB}
  ${GTEST_LIBS}
  ${CMAKE_DL_LIBS}
  ${GMP_LIB}
)



# On Linux if using curl we need to add ssl & crypto.
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(
    automaton-core
    ssl crypto z
  )
endif()

# On Mac OS X if using curl we need to also link to LDAP.
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  target_link_libraries(
    automaton-core
    ssl crypto z LDAP
  )
endif()

# On Windows if using curl we need to also link to LDAP, Wincrypt, Winnls, Winsock.
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(
    automaton-core
    crypt32 Normaliz wldap32 ws2_32
  )
endif()

macro(automaton_configure_debugger_directory target_name)
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set_property(TARGET ${target_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endmacro()

add_executable(automaton-core-cli automaton/core/core.cc)
target_compile_definitions(
  automaton-core-cli PUBLIC
  BOOST_ALL_NO_LIB
  CURL_STATICLIB
)
target_link_libraries(automaton-core-cli automaton-core)
automaton_configure_debugger_directory(automaton-core-cli)

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
  target_include_directories(${test_name} PRIVATE ${GTEST_INCLUDE_DIR})
  target_link_libraries(${test_name} automaton-core ${GTEST_LIBS})
  add_test(
    NAME ${test_name}
    COMMAND ${test_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  automaton_configure_debugger_directory(${test_name})
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

# TODO(akovachev): test is flaky, but we should fix that
# automaton_test(miner miner_test)

# automaton_test(network rpc_server_test)
# automaton_test(network http_server_test)

automaton_test(script test_script)

if(automaton_RUN_GANACHE_TESTS)
  automaton_test(interop abi_encoder_test)
endif()

# Memory mapped file tests seem to be flaky on Windows
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  automaton_test(state state_test)
  automaton_test(state state_persistent_test)
  automaton_test(storage persistent_blobstore_test)
  automaton_test(storage blobstore_test)
  # automaton_test(storage persistent_storage_test)
  automaton_test(storage persistent_vector_test)
endif()

add_library(
  blockchain_cpp_node STATIC
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
  koh-miner STATIC
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
  eth_contract_raw STATIC
  automaton/core/interop/ethereum/eth_contract_raw.cc
)

target_link_libraries(
  eth_contract_raw
  automaton-core
)

add_executable(
  eth_contract_raw_test
  automaton/core/interop/ethereum/eth_contract_raw_test.cc
)

target_link_libraries(
  eth_contract_raw_test
  eth_contract_raw
)

add_executable(
  eth_contract_curl_test
  automaton/core/interop/ethereum/eth_contract_curl_test.cc
)

target_link_libraries(
  eth_contract_curl_test
  automaton-core
)

add_executable(
  contract_functions_test
  automaton/tests/interop/contract_functions_test.cc
)

target_link_libraries(
  contract_functions_test
  automaton-core
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
target_compile_definitions(
  automaton-core PUBLIC
  CURL_STATICLIB
  BOOST_ALL_NO_LIB
  NOGDI
  _WIN32_WINNT=0x0A00
)
endif()

install(TARGETS automaton-core;blockchain_cpp_node
  ARCHIVE
    DESTINATION lib
  PUBLIC_HEADER
    DESTINATION include
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/automaton/
  DESTINATION include/automaton
  FILES_MATCHING PATTERN "*.h"
  PATTERN "automaton/tests" EXCLUDE
)

if (automaton_BUILD_DEPENDENCIES)
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  include(BuildDependencies)
  add_dependencies(automaton-core
    ext_replxx
    ext_cryptopp
    ext_g3log
    ed25519_install
    ed25519
    ext_protobuf
    ext_curl
    ext_googletest
    ext_boost
    ext_sol2
    lua_install
    json_install
  )
endif()
