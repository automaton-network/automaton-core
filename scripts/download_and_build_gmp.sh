GMPVER="6.1.2"

# Downlaod gmp
[ ! -d gmp ] && \
  get_archive "https://gmplib.org/download/gmp/gmp-$GMPVER.tar.xz" \
  "gmp-$GMPVER.tar.xz" "87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912"
[ -d "gmp-$GMPVER" ] && mv "gmp-$GMPVER" gmp

# Build gmp
if [ ! -f gmp/.libs/libgmp.a ]; then
  print_separator "=" 80
  echo "  BUILDING gmp"
  print_separator "=" 80

  cd gmp
  ./configure
  make -j$CPUCOUNT
  cd ..
else
  print_separator "=" 80
  echo "  gmp ALREADY BUILT"
  print_separator "=" 80
fi
