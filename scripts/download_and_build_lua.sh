LUA_VER="5.3.5"

# Download Lua
[ ! -d lua ] && \
  get_archive "https://www.lua.org/ftp/lua-$LUA_VER.tar.gz" \
  "lua-$LUA_VER.tar.gz" "0c2eed3f960446e1a3e4b9a1ca2f3ff893b6ce41942cf54d5dd59ab4b3b058ac"
[ -d lua-$LUA_VER ] && mv lua-$LUA_VER/src lua && rm -rf lua-$LUA_VER

# Build Lua
if [ ! -f lua/liblua.a ]; then
  print_separator "=" 80
  echo "  BUILDING Lua"
  print_separator "=" 80

  cd lua
  if $darwin; then
    make -j$CPUCOUNT macosx a
  else
    make -j$CPUCOUNT linux a
  fi
  cd ..
else
  print_separator "=" 80
  echo "  Lua ALREADY BUILT"
  print_separator "=" 80
fi
