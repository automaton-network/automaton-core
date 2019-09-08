JUCE_VER="5.4.4"

# Download JUCE
[ ! -d JUCE ] && \
  get_archive "https://github.com/WeAreROLI/JUCE/archive/$JUCE_VER.tar.gz" \
  "JUCE-$JUCE_VER.tar.gz" "40536c5792002ceec09faea8d5a57d5c17486f8805c5efe3bc734e27d2df2202"
[ -d JUCE-$JUCE_VER ] && mv JUCE-$JUCE_VER JUCE

# Build JUCE Projucer -- only when not in CI
if [ ! -f JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer ]; then
  if [ ! $CI ]; then
    print_separator "=" 80
    echo "  BUILDING Projucer"
    print_separator "=" 80

    if $darwin; then
      # defaults write com.apple.dt.Xcode IDEBuildOperationMaxNumberOfConcurrentCompileTasks `sysctl -n hw.ncpu`
      xcodebuild \
        -configuration Release \
        GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS JUCER_ENABLE_GPL_MODE=1' \
        -project ./JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj
    else
      cd JUCE/extras/Projucer/Builds/LinuxMakefile
      make -j$CPUCOUNT CPPFLAGS="-DJUCER_ENABLE_GPL_MODE=1" CONFIG=Release # V=1 for verbose
      cd ../../../../..
    fi
  fi
else
  print_separator "=" 80
  echo "  Projucer ALREADY BUILT"
  print_separator "=" 80
fi
