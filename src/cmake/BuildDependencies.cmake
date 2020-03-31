include(ExternalProject)
include (FetchContent)
include(ProcessorCount)
ProcessorCount(CPUCOUNT)
if(NOT CPUCOUNT EQUAL 0)
  set(CTEST_BUILD_FLAGS -j${CPUCOUNT})
  set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${CPUCOUNT})
else()
  set(CPUCOUNT 1)
endif()

set(FETCHCONTENT_QUIET OFF)

ExternalProject_Add(ext_replxx
  GIT_REPOSITORY "https://github.com/AmokHuginnsson/replxx.git"
  GIT_TAG "3cb884e3fb4b1a28efeb716fac75f77eecc7ea3d"
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  CMAKE_CACHE_ARGS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY} -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
  UPDATE_COMMAND ""
)

FetchContent_Declare(cryptopp_cmake
  GIT_REPOSITORY "https://github.com/noloader/cryptopp-cmake.git"
  GIT_TAG "85941c65aebe7e776edd0f7015f151464f2f19e4"
  UPDATE_COMMAND ""
)

FetchContent_Declare(ed25519_source
  GIT_REPOSITORY "https://github.com/orlp/ed25519.git"
  GIT_TAG "7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"
  UPDATE_COMMAND ""
)

FetchContent_Declare(json
  GIT_REPOSITORY "https://github.com/nlohmann/json.git"
  GIT_TAG "v3.2.0"
  UPDATE_COMMAND ""
)

FetchContent_Declare(lua_source
  GIT_REPOSITORY "https://github.com/lua/lua.git"
  GIT_TAG "e354c6355e7f48e087678ec49e340ca0696725b1"
  UPDATE_COMMAND
)

FetchContent_GetProperties(cryptopp_cmake)
if (NOT cryptopp_cmake_POPULATED)
  FetchContent_Populate(cryptopp_cmake)
endif()

ExternalProject_Add(ext_cryptopp
  GIT_REPOSITORY "https://github.com/weidai11/cryptopp.git"
  GIT_TAG "51b2df1012c9a6ee419daafb339e83ffb9d145e7"
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${cryptopp_cmake_SOURCE_DIR}/CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${cryptopp_cmake_SOURCE_DIR}/cryptopp-config.cmake <SOURCE_DIR>/cryptopp-config.cmake
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_SHARED=OFF -DBUILD_TESTING=OFF
  CMAKE_CACHE_ARGS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY} -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
  UPDATE_COMMAND ""
)

FetchContent_GetProperties(ed25519_source)
if (NOT ed25519_source_POPULATED)
  FetchContent_Populate(ed25519_source)
endif()

FetchContent_GetProperties(json)
if (NOT json_POPULATED)
  FetchContent_Populate(json)
endif()

FetchContent_GetProperties(lua_source)
if (NOT lua_sources_POPULATED)
  FetchContent_Populate(lua_source)
endif()

add_custom_target(json_install
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${json_SOURCE_DIR}/single_include/nlohmann/json.hpp ${CMAKE_INSTALL_PREFIX}/include/json.hpp
)

file(WRITE ${CMAKE_INSTALL_PREFIX}/include/lua.hpp
"extern \"C\" {\n"
"#include \"lua.h\"\n"
"#include \"lualib.h\"\n"
"#include \"lauxlib.h\"\n"
"}\n"
)

file(GLOB ED25519_SRC ${CMAKE_BINARY_DIR}/_deps/ed25519_source-src/src/*.c)
add_library(ed25519 STATIC ${ED25519_SRC})
add_custom_target(ed25519_install
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ed25519_source_SOURCE_DIR}/src/ed25519.h ${CMAKE_INSTALL_PREFIX}/include/ed25519.h
)

install(TARGETS ed25519
  ARCHIVE
    DESTINATION lib
)

ExternalProject_Add(ext_zlib
  URL "http://www.zlib.net/zlib1211.zip"
  URL_HASH SHA256=d7510a8ee1918b7d0cad197a089c0a2cd4d6df05fee22389f67f115e738b178d
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_SHARED_LIBS=OFF
  CMAKE_CACHE_ARGS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY} -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
)

ExternalProject_Add(ext_protobuf
  GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
  GIT_TAG "v3.11.4"
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  SOURCE_SUBDIR cmake
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON -Dprotobuf_MSVC_STATIC_RUNTIME=${automaton_STATIC_RUNTIME} -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=OFF
  UPDATE_COMMAND ""
)

string(TOLOWER ${CMAKE_BUILD_TYPE} BOOST_VARIANT)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BOOST_ADDRESS_MODEL 64)
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(OPENSSL_PLATFORM VC-WIN64A-masm)
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OPENSSL_PLATFORM linux-x86_64)
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OPENSSL_PLATFORM darwin64-x86_64-cc)
  endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(BOOST_ADDRESS_MODEL 32)
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(OPENSSL_PLATFORM VC-WIN32)
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OPENSSL_PLATFORM linux-x86)
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OPENSSL_PLATFORM darwin-i386-cc)
  endif()
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(BOOST_BOOTSTRAP_COMMAND bootstrap.bat)
  if (automaton_STATIC_RUNTIME)
    set (BOOST_RUNTIME_LINKING runtime-link=static)
    set (GTEST_SHARED_CRT -Dgtest_force_shared_crt=OFF)
    set (G3_SHARED_CRT -DG3_SHARED_RUNTIME=OFF)
  else()
    set (BOOST_RUNTIME_LINKING runtime-link=shared)
    set (GTEST_SHARED_CRT -Dgtest_force_shared_crt=ON)
    set (G3_SHARED_CRT -DG3_SHARED_RUNTIME=ON)
  endif()
  set(OPENSSL_MAKE_COMMAND nmake)
else()
  set(BOOST_BOOTSTRAP_COMMAND bootstrap.sh)
  set(OPENSSL_MAKE_COMMAND make -j${CPUCOUNT})
endif()

ExternalProject_Add(ext_boost
  URL "https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.zip"
  URL_HASH SHA256=48F379B2E90DD1084429AAE87D6BDBDE9670139FA7569EE856C8C86DD366039D
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/${BOOST_BOOTSTRAP_COMMAND}
  BUILD_COMMAND <SOURCE_DIR>/b2 --prefix=${CMAKE_INSTALL_PREFIX} --layout=system --with-date_time --with-filesystem --with-system --with-iostreams cxxstd=14 ${BOOST_RUNTIME_LINKING} link=static variant=${BOOST_VARIANT} address-model=${BOOST_ADDRESS_MODEL} -s ZLIB_INCLUDE="${CMAKE_INSTALL_PREFIX}/include" -s ZLIB_BINARY=${Z_LIB} -s ZLIB_LIBPATH="${CMAKE_INSTALL_PREFIX}/lib" -s NO_BZIP2=1 install -j${CPUCOUNT}
  INSTALL_COMMAND ""
  DEPENDS ext_zlib
)

file(GLOB LUA_SRC ${CMAKE_BINARY_DIR}/_deps/lua_source-src/*.c)
add_library(lua STATIC ${LUA_SRC})
add_custom_target(lua_install
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/lua_source-src/lua.h ${CMAKE_INSTALL_PREFIX}/include/lua.h
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/lua_source-src/luaconf.h ${CMAKE_INSTALL_PREFIX}/include/luaconf.h
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/lua_source-src/lualib.h ${CMAKE_INSTALL_PREFIX}/include/lualib.h
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/lua_source-src/lauxlib.h ${CMAKE_INSTALL_PREFIX}/include/lauxlib.h
)

install(TARGETS lua;ed25519
  ARCHIVE
    DESTINATION lib
  PUBLIC_HEADER
    DESTINATION include
)

ExternalProject_Add(ext_googletest
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "2fe3bd994b3189899d93f1d5a881e725e046fdc2"
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${GTEST_SHARED_CRT}
  UPDATE_COMMAND ""
)

ExternalProject_Add(ext_g3log
  GIT_REPOSITORY "https://github.com/KjellKod/g3log.git"
  GIT_TAG "4000c5c899c0ae58b8b851f9b66e1a2ac0fe2bff"
  UPDATE_COMMAND ""
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DG3_SHARED_LIB=OFF -DCHANGE_G3LOG_DEBUG_TO_DBUG=ON -DUSE_DYNAMIC_LOGGING_LEVELS=ON ${G3_SHARED_CRT}
)

if (automaton_USE_OPENSSL)
  ExternalProject_Add(ext_openssl
    URL "https://github.com/openssl/openssl/archive/OpenSSL_1_1_1d.zip"
    URL_HASH SHA256=a366e3b6d8269b5e563dabcdfe7366d15cb369517f05bfa66f6864c2a60e39e8
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND perl Configure --prefix=${CMAKE_INSTALL_PREFIX} --openssldir=${CMAKE_INSTALL_PREFIX}/ssl ${OPENSSL_PLATFORM} no-tests no-asm no-shared
    BUILD_COMMAND ${OPENSSL_MAKE_COMMAND}
    INSTALL_COMMAND ${OPENSSL_MAKE_COMMAND} install_sw
  )
  set(SSL_CURL_FLAGS -DCMAKE_USE_OPENSSL=ON -DOPENSSL_ROOT_DIR=${CMAKE_INSTALL_PREFIX} -DOPENSSL_USE_STATIC_LIBS=TRUE)
else()
  set(SSL_CURL_FLAGS -DCMAKE_USE_WINSSL=ON)
endif()


ExternalProject_Add(ext_curl
  URL "https://github.com/curl/curl/releases/download/curl-7_65_3/curl-7.65.3.zip"
  URL_HASH SHA256=F8934D25C8FCC01ABBE9B846F70523E3DBB5386BBE82C3C213913F6B469787AF
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_SHARED_LIBS=OFF -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF ${SSL_CURL_FLAGS} -DHTTP_ONLY=ON -DCMAKE_USE_LIBSSH2=OFF
  CMAKE_CACHE_ARGS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY} -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
  UPDATE_COMMAND ""
)

if (automaton_USE_OPENSSL)
  add_dependencies(ext_curl ext_openssl)
endif()

ExternalProject_Add(ext_sol2
  GIT_REPOSITORY "https://github.com/ThePhD/sol2.git"
  GIT_TAG "254466eb4b3ae630c731a557987f3adb1a8f86b0"
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  CMAKE_CACHE_ARGS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY} -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
  UPDATE_COMMAND ""
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  FetchContent_Declare(secp256k1_source
    GIT_REPOSITORY "https://github.com/bitcoin/bitcoin.git"
    GIT_TAG "1bc9988993ee84bc814e5a7f33cc90f670a19f6a"
    UPDATE_COMMAND ""
  )
  FetchContent_GetProperties(secp256k1_source)
  if (NOT secp256k1_source_POPULATED)
    FetchContent_Populate(secp256k1_source)
  endif()
  add_library(secp256k1 STATIC
    ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/src/secp256k1.c
    ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/src/modules/recovery/main_impl.h
  )
  add_custom_target(secp256k1_config
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/build_msvc/libsecp256k1_config.h ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/src/libsecp256k1-config.h
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/include/secp256k1.h ${CMAKE_INSTALL_PREFIX}/include/secp256k1.h
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/include/secp256k1_recovery.h ${CMAKE_INSTALL_PREFIX}/include/secp256k1_recovery.h
  )
  add_dependencies(secp256k1 secp256k1_config)
  target_include_directories(secp256k1 PRIVATE ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1)
  target_include_directories(secp256k1 PRIVATE ${CMAKE_BINARY_DIR}/_deps/secp256k1_source-src/src/secp256k1/src)
  target_compile_definitions(secp256k1 PUBLIC
    ZMQ_STATIC
   _CONSOLE
   ENABLE_MODULE_RECOVERY
   HAVE_CONFIG_H
  )

  target_compile_definitions(secp256k1 PUBLIC
   NOMINMAX
   WIN32
   _CRT_SECURE_NO_WARNINGS
   _SCL_SECURE_NO_WARNINGS
   )
  install(TARGETS secp256k1
    ARCHIVE
      DESTINATION lib
    PUBLIC_HEADER
      DESTINATION include
  )
else()
  ExternalProject_Add(ext_gmp
    URL "https://gmplib.org/download/gmp/gmp-6.1.2.tar.xz"
    URL_HASH SHA256=87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX} --enable-shared=no
    BUILD_COMMAND make -j${CPUCOUNT}
    INSTALL_COMMAND make -j${CPUCOUNT} install
  )
  ExternalProject_Add(ext_secp256k1
    GIT_REPOSITORY "https://github.com/bitcoin-core/secp256k1"
    GIT_TAG "e9fccd4de1f2b382545dfbadeae54868447e2cdf"
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./autogen.sh COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX} --enable-module-recovery --enable-shared=no --with-sysroot=${CMAKE_INSTALL_PREFIX}
    BUILD_COMMAND make -j${CPUCOUNT}
    INSTALL_COMMAND make -j${CPUCOUNT} install
    UPDATE_COMMAND ""
    DEPENDS ext_gmp
  )
endif()

FetchContent_GetProperties(JUCE)

if (NOT JUCE_POPULATED)
  FetchContent_Populate(JUCE)
endif()
