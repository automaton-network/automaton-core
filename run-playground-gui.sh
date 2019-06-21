#!/bin/bash

darwin=false;
case "`uname`" in
  Darwin*) darwin=true ;;
esac

if $darwin; then
  sedi="sed -i ''"
  CPUCOUNT=$(sysctl -n hw.ncpu)
else
  sedi="sed -i "
  CPUCOUNT=$(grep -c "^processor" /proc/cpuinfo)
fi

echo "$CPUCOUNT logical cores"

if $darwin; then
  # defaults write com.apple.dt.Xcode IDEBuildOperationMaxNumberOfConcurrentCompileTasks `sysctl -n hw.ncpu`
  xcodebuild \
    -configuration Release \
    -project ./src/automaton/playground/PlaygroundGUI/Builds/MacOSX/PlaygroundGUI.xcodeproj \
  && ./src/automaton/playground/PlaygroundGUI/Builds/MacOSX/build/Release/PlaygroundGUI.app/Contents/MacOS/PlaygroundGUI
else
  cd src/automaton/playground/PlaygroundGUI/Builds/LinuxMakefile
  make CONFIG=Release -j$CPUCOUNT && ./build/PlaygroundGUI
fi
