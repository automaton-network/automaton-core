#!/bin/bash
CPUCOUNT=$(grep -c "^processor" /proc/cpuinfo)

cd src/automaton/playground/PlaygroundGUI/Builds/LinuxMakefile
make CONFIG=Release -j$CPUCOUNT && ./build/PlaygroundGUI
