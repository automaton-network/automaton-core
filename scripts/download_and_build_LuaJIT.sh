LUAJIT_VER="2.0.5"

# Download LuaJIT
[ ! -d LuaJIT ] && \
  get_archive "https://github.com/LuaJIT/LuaJIT/archive/v$LUAJIT_VER.tar.gz" \
  "LuaJIT-$LUAJIT_VER.tar.gz" "8bb29d84f06eb23c7ea4aa4794dbb248ede9fcb23b6989cbef81dc79352afc97"
[ -d LuaJIT-$LUAJIT_VER ] && mv LuaJIT-$LUAJIT_VER LuaJIT

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
