# To run this script:
# 1. Stat Developer Command Prompt for VS
# 2. Type powershell
# 3. Run the script from its directory

$LOCAL_3P="local_third_party"
cd .\src\
New-Item -ItemType Directory -Path .\$LOCAL_3P
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

function Get-Archive($url, $filename, $sha) {
  echo ("="*80)
  echo "  Downloading $filename"
  echo ("="*80)
  if (!(Test-Path .\$filename)) {
    wget -URI $url -OutFile $filename
  }
  $filehash = (Get-FileHash -Path .\$filename -Algorithm SHA256).Hash
  if($filehash -ne $sha) {
    echo "Error: Wrong hash [$filehash] Expected [$sha]"
  } else {
    echo "Extracting $filename"
    Expand-Archive -Path $filename -DestinationPath .\ -Force
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
Get-GitRepo "https://github.com/protocolbuffers/protobuf.git" "protobuf" "48cb18e5c419ddd23d9badcfe4e9df7bde1979b2"
# Get-GitRepo "https://github.com/svaarala/duktape.git" "duktape" "d7fdb67f18561a50e06bafd196c6b423af9ad6fe"

#  ====== Check if missing and download using wget ======

if(!(Test-Path -Path .\json-3.1.2) -and (New-Item -ItemType Directory -Path .\json-3.1.2)) {
  wget -URI https://github.com/nlohmann/json/releases/download/v3.2.0/json.hpp -OutFile json-3.1.2/json.hpp
}

#if(!(Test-Path -Path .\boost_1_68_0)) {
#    Get-Archive "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip" `
#  "boost_1_68_0.zip" "3B1DB0B67079266C40B98329D85916E910BBADFC3DB3E860C049056788D4D5CD"
#}

if(!(Test-Path -Path .\zlib-1.2.11)) {
    Get-Archive "https://zlib.net/zlib1211.zip" `
  "zlib-1.2.11.zip" "d7510a8ee1918b7d0cad197a089c0a2cd4d6df05fee22389f67f115e738b178d"
}

if(!(Test-Path -Path .\bzip2-1.0.6)) {
    Get-Archive "https://fossies.org/linux/misc/bzip2-1.0.6.zip" `
  "bzip2-1.0.6.zip" "C60C0ABC89A76C170B3EF39E7C516DBDEA806D65231204C1803A610FA9DBC464"
}

#if(!(Test-Path -Path .\xz-5.2.3)) {
#    Get-Archive "http://phoenixnap.dl.sourceforge.net/project/lzmautils/xz-5.2.3.tar.gz" `
#  "xz-5.2.3.tar.gz" "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb"
#}

#  ======== Build libs ========

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

#  ====== Building LuaJIT ======
# echo ("="*80)
# echo "  BUILDING LuaJIT"
# echo ("="*80)

# cd LuaJIT\src
# .\msvcbuild.bat
# cd ..\..

#  ====== Building libzmq ======
echo ("="*80)
echo "  BUILDING libzmq"
echo ("="*80)

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
New-Item -ItemType Directory -Path .\build
cd build
if (!(Test-Path .\CMakeCache.txt)) {
    cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release ..
}
msbuild.exe replxx.sln /p:configuration=Release
cd ..\..

#  ====== Building boost ======
echo ("="*80)
echo "  BUILDING boost"
echo ("="*80)

if ($APPVEYOR) {
  echo "Moving pre-installed boost into our tree"
  Move-Item -Path C:\Libraries\boost_1_67_0 -Destination .\boost_1_68_0
  cd boost_1_68_0
  mkdir stage
  cd stage
  Move-Item -Path ..\lib64-msvc-14.1 -Destination lib
  cd ..\..
} else {
  cd boost_1_68_0
  .\bootstrap.bat
  .\b2 --with-filesystem --with-system --with-iostreams cxxstd=14 link=static stage
  cd ..
}

echo "Done."
