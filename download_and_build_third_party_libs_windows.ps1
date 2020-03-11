# Download&Build3P
# To run this script:
# 1. Stat Developer Command Prompt for VS
# 2. Type powershell
# 3. Run the script from its directory

# To build the CMake, make a automaton/src/build directory and cd to it
# then from inside do the following commands:
# cmake ..
# msbuild -m automaton-network.sln /p:platform="x64" /p:Configuration=Release>build.txt

$LOCAL_3P="local_third_party"

$BOOST_VER_U="1_70_0"
$BOOST_VER="1.70.0"
$CURL_VER_U="7_65_3"
$CURL_VER="7.65.3"
$G3LOG_VER="1.3.2"
$JSON_VER="3.2.0"
$OPENSSL_VER_U="1_0_2u"
$ZLIB_VER="1.2.11"

$START_PATH=$(Get-Location)

cd .\src\
if (!(Test-Path -Path .\$LOCAL_3P)) {
  New-Item -ItemType Directory -Path .\$LOCAL_3P
}
cd .\$LOCAL_3P
[Net.ServicePointManager]::SecurityProtocol = "tls12, tls11"

function Get-GitRepo($repo, $dir, $commit) {
  echo ("="*80)
  echo " Updating $dir from repo $repo "
  echo ("="*80)

  if(!(Test-Path -Path .\$dir)) {
    git clone $repo $dir
  }

  cd $dir

  if($commit) {
    git reset --hard $commit
  } else {
    git pull
  }

  cd ..

}

function Exit-Now() {
  cd $START_PATH
  exit
}

function Get-Archive($url, $filename, $sha) {
  echo ("="*80)
  echo "  Downloading $filename"
  echo ("="*80)
  if (!(Test-Path .\$filename)) {
    wget -URI $url -OutFile $filename
  }
  $filehash = (Get-FileHash -Path .\$filename -Algorithm SHA256).Hash
  if($filehash -ne $sha) {
    Write-Host "Error: Wrong hash [$filehash] Expected [$sha]" -ForegroundColor Red
    Exit-Now
  } else {
    echo "Extracting $filename"
    Expand-7Zip $filename .\
    echo "done"
  }
}

#  ========  Download all libraries ========
Get-GitRepo "https://github.com/LuaJIT/LuaJIT.git" "LuaJIT" "0bf80b07b0672ce874feedcc777afe1b791ccb5a"
# Get-GitRepo "https://github.com/zeromq/libzmq.git" "libzmq" "d062edd8c142384792955796329baf1e5a3377cd"
# Get-GitRepo "https://github.com/zeromq/cppzmq.git" "cppzmq" "d9f0f016c07046742738c65e1eb84722ae32d7d4"
# Get-GitRepo "https://github.com/zeromq/zmqpp.git" "zmqpp" "f8ff127683dc555aa004c0e6e2b18d2354a375be"
Get-GitRepo "https://github.com/ThePhD/sol2.git" "sol2" "254466eb4b3ae630c731a557987f3adb1a8f86b0"
Get-GitRepo "https://github.com/AmokHuginnsson/replxx.git" "replxx" "3cb884e3fb4b1a28efeb716fac75f77eecc7ea3d"
Get-GitRepo "https://github.com/lua/lua.git" "lua" "e354c6355e7f48e087678ec49e340ca0696725b1"
Get-GitRepo "https://github.com/muflihun/easyloggingpp.git" "easyloggingpp" "a5317986d74b6dd3956021cb7fbb0669cce398b2"
Get-GitRepo "https://github.com/weidai11/cryptopp.git" "cryptopp" "c8d8caf70074655a2562ae1ea45cb30e28fee2b4"
Get-GitRepo "https://github.com/orlp/ed25519.git" "ed25519" "7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"
Get-GitRepo "https://github.com/google/googletest.git" "googletest" "2fe3bd994b3189899d93f1d5a881e725e046fdc2"
# Get-GitRepo "https://github.com/nlohmann/json.git" "json" "359f98d14065bf4e53eeb274f5987fd08f16e5bf"
# Get-GitRepo "https://github.com/nelhage/rules_boost.git" "com_github_nelhage_rules_boost" "fe787183c14f2a5c6e5e1e75a7c57d2e799d3d19"
Get-GitRepo "https://github.com/protocolbuffers/protobuf.git" "protobuf" "b829ff2a4614ff25048944b2cdc8e43b6488fda0"
# Get-GitRepo "https://github.com/svaarala/duktape.git" "duktape" "d7fdb67f18561a50e06bafd196c6b423af9ad6fe"
Get-GitRepo "https://github.com/bitcoin/bitcoin" "bitcoin" "1bc9988993ee84bc814e5a7f33cc90f670a19f6a"

#  ====== Check if missing and download using wget ======

if(!(Test-Path -Path .\json)) {
  New-Item -ItemType Directory -Path .\json
  wget -URI https://github.com/nlohmann/json/releases/download/v$JSON_VER/json.hpp -OutFile json/json.hpp
}

if(!(Test-Path -Path .\zlib)) {
  Get-Archive "https://zlib.net/zlib1211.zip" `
    "zlib-$ZLIB_VER.zip" "d7510a8ee1918b7d0cad197a089c0a2cd4d6df05fee22389f67f115e738b178d"
  Rename-Item zlib-$ZLIB_VER zlib
}

if(!(Test-Path -Path .\bzip2-1.0.6)) {
    Get-Archive "https://github.com/nemequ/bzip2/archive/v1.0.6.zip" `
  "bzip2-1.0.6.zip" "1AC730150D4C13A6933101C8D21ACC6DE258503AE8A6A049E948A47749DDCC81"
}

if(!(Test-Path -Path .\g3log)) {
  Get-Archive "https://github.com/KjellKod/g3log/archive/$G3LOG_VER.zip" `
    "g3log-$G3LOG_VER.zip" "1c141aa62c30985e8fd8c56bddbf2e32f080bf839a48f53c9593ecdebfb8a175"
}

if(Test-Path -Path g3log-$G3LOG_VER) {
  Rename-Item g3log-$G3LOG_VER g3log
}

if(!(Test-Path -Path .\curl)) {
  Get-Archive "https://github.com/curl/curl/releases/download/curl-$CURL_VER_U/curl-$CURL_VER.zip" `
      "curl-$CURL_VER.zip" "F8934D25C8FCC01ABBE9B846F70523E3DBB5386BBE82C3C213913F6B469787AF"
  Rename-Item curl-$CURL_VER curl
}

#if(!(Test-Path -Path .\xz-5.2.3)) {
#    Get-Archive "http://phoenixnap.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz" `
#  "xz-5.2.3.tar.gz" "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb"
#}

#  ======== Build libs ========


#  ====== Build cryptlib ======
echo ("="*80)
echo "  BUILDING cryptopp"
echo ("="*80)

cd cryptopp
msbuild -m /t:Build /p:platform="x64" /p:configuration=Release cryptlib.vcxproj
cd ..

#  ====== Build protobuf ======
echo ("="*80)
echo "  BUILDING protobuf"
echo ("="*80)

cd protobuf
if(!(Test-Path -Path .\build_msvc)) {
  mkdir build_msvc
}
cd build_msvc
cmake -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_GENERATOR_PLATFORM=x64 ../cmake
msbuild protobuf.sln /p:Configuration=Release
cd ..\..

#  ====== Build Bitcoin libraries ======
echo ("="*80)
echo "  BUILDING Bitcoin libraries"
echo ("="*80)
cd bitcoin\build_msvc
python msvc-autogen.py
set "INCLUDE=$START_PATH\src\local_third_party\boost;%INCLUDE%"
set UseEnv=true
msbuild /m /t:Build libsecp256k1\libsecp256k1.vcxproj /p:Configuration=Release
cd ..\..

#  ====== Build g3log ======
echo ("="*80)
echo "  BUILDING g3log"
echo ("="*80)

cd .\g3log
if(!(Test-Path -Path .\build)) {
  md build
}
cd build
cmake .. -DCMAKE_CXX_FLAGS_RELEASE="/MT" -DG3_SHARED_LIB=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_GENERATOR_PLATFORM=x64 -DCHANGE_G3LOG_DEBUG_TO_DBUG=ON -DUSE_DYNAMIC_LOGGING_LEVELS=ON
cmake --build . --config Release
cd ..\..

#  ====== Create lua.hpp with extern includes ======
echo ("="*80)
echo "  Create lua.hpp"
echo ("="*80)

$lua_extern_includes = "extern `"C`" {
#include `"lua.h`"
#include `"lualib.h`"
#include `"lauxlib.h`"
}"
$lua_extern_includes | Out-File -FilePath .\lua\lua.hpp -Encoding utf8
$lua_extern_includes | Out-File -FilePath .\LuaJIT\src\lua.hpp -Encoding utf8

#  ====== Building Lua ======
# echo ("="*80)
# echo "  BUILDING Lua"
# echo ("="*80)

cd lua
cl /MT /O2 /c *.c
lib /OUT:lua.lib *.obj
cd ..

#  ====== Building LuaJIT ======
# echo ("="*80)
# echo "  BUILDING LuaJIT"
# echo ("="*80)

cd LuaJIT\src
.\msvcbuild.bat static
cd ..\..

#  ====== Building libzmq ======
# echo ("="*80)
# echo "  BUILDING libzmq"
# echo ("="*80)

#print_separator "=" 80
#echo "   BUILDING libzmq"
#print_separator "=" 80
#
#unset GREP_COLOR
#unset GREP_OPTIONS
#cd libzmq
#[ ! -f configure ] && ./autogen.sh && ./configure
#make
#cd ..

#  ====== Building zmqpp ======
#echo ("="*80)
#echo "  BUILDING zmqpp"
#echo ("="*80)

#cd zmqpp
#$sedi 's/CUSTOM_INCLUDE_PATH =/CUSTOM_INCLUDE_PATH = -I..\/libzmq\/include/' Makefile
#$sedi 's/LIBRARY_LIBS =/LIBRARY_LIBS = -L..\/libzmq\/src\/.libs/' Makefile
#make
#cd ..

#  ====== Building replxx ======
echo ("="*80)
echo "  BUILDING replxx"
echo ("="*80)

cd replxx
if(!(Test-Path -Path .\build)) {
  New-Item -ItemType Directory -Path .\build
}
cd build
if (!(Test-Path .\CMakeCache.txt)) {
  cmake -DCMAKE_BUILD_TYPE=Release ..
}
msbuild.exe replxx.sln /p:configuration=Release
cd ..\..

#  ====== Building boost ======
echo ("="*80)
echo "  BUILDING boost"
echo ("="*80)

if ($env:APPVEYOR) {
  echo "Moving pre-installed boost into our tree"
  Move-Item -Path C:\Libraries\boost_$BOOST_VER_U -Destination .\boost
  cd boost
  mkdir stage
  cd stage
  Move-Item -Path ..\lib64-msvc-14.1 -Destination lib
  cd ..\..
} else {
  if(!(Test-Path -Path .\boost)) {
    Get-Archive "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_$BOOST_VER_U.zip" `
        "boost_$BOOST_VER_U.zip" "48F379B2E90DD1084429AAE87D6BDBDE9670139FA7569EE856C8C86DD366039D"
    Rename-Item boost_$BOOST_VER_U boost
  }
  if(!(Test-Path -Path .\boost\stage\lib)) {
    cd boost
    .\bootstrap.bat
    .\b2 --with-filesystem --with-system --with-iostreams cxxstd=14 runtime-link=static link=static stage
    cd ..
  }
}

#  ====== Building zlib  ======
echo ("="*80)
echo "  BUILDING zlib"
echo ("="*80)
if (!(Test-Path -Path .\zlib\zlib.lib)) {
  cd zlib
  nmake zlib.lib -f win32/Makefile.msc AS=ml64 LOC="-DASMV -DASMINF -I." OBJA="inffasx64.obj gvmat64.obj inffas8664.obj"
  cd ..
}

#  ====== Building OpenSSL ======
echo ("="*80)
echo "  BUILDING OpenSSL"
echo ("="*80)

if (!(Test-Path -Path .\openssl)) {
    Get-Archive "https://github.com/openssl/openssl/archive/OpenSSL_$OPENSSL_VER_U.zip" `
  "openssl-OpenSSL_$OPENSSL_VER_U.zip" "493F8B34574D0CF8598ADBDEC33C84B8A06F0617787C3710D20827C01291C09C"
  Rename-Item openssl-OpenSSL_$OPENSSL_VER_U openssl
}
if (!(Test-Path -Path .\openssl\out32\libeay32.lib)) {
  cd openssl
  perl Configure VC-WIN64A
  ms\do_win64a
  nmake -f ms\nt.mak
  cd ..
}

#  ====== Building curl ======
echo ("="*80)
echo "  BUILDING curl"
echo ("="*80)

if (Test-Path -Path .\curl\winbuild) {
  cd curl\winbuild
  if (!(Test-Path -Path lib)) {
    mkdir lib
  }
  cmd /c copy /y ..\..\zlib\*.lib lib
  cmd /c copy /y ..\..\openssl\out32\*.lib lib
  if (!(Test-Path -Path include)) {
    mkdir include
  }
  cmd /c xcopy /y /e ..\..\openssl\inc32 include
  cmd /c copy /y ..\..\zlib\*.h include
  nmake /f Makefile.vc mode=static WITH_DEVEL=. WITH_SSL=static WITH_ZLIB=static MACHINE=x64 SSL_PATH=. ZLIB_PATH=.
  cd ../..
}

cd $START_PATH
echo "Done."

# OpenSSL is used by libcurl so needs to be built prior.
# . $AUTOMATON_ROOT/scripts/download_and_build_openssl.sh
#
# . $AUTOMATON_ROOT/scripts/download_and_build_boost.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_cryptopp.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_curl.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_ed25519.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_g3log.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_gmp.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_googletest.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_JUCE.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_lua.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_LuaJIT.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_pcre.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_protobuf.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_replxx.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_secp256k1.sh
# . $AUTOMATON_ROOT/scripts/download_and_build_zlib.sh
