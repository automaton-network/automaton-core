JUCE_VER="5.4.5"

# Download JUCE
[ ! -d JUCE ] && \
  get_archive "https://github.com/WeAreROLI/JUCE/archive/$JUCE_VER.tar.gz" \
  "JUCE-$JUCE_VER.tar.gz" "e51019065f1185db124954959aeb9101d759a4f2a4dd0c4a6b305a37f0c9271f"
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
