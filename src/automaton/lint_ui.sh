#!/bin/bash
function join_by { local IFS="$1"; shift; echo "$*"; }

FILTERS_UI=(
  "-build/c++11"
  "-build/header_guard"
  "-build/include"
  "-legal/copyright"
  "-runtime/explicit"
)

FILTER_ARG_UI=$(join_by "," "${FILTERS_UI[@]}")

find playground -path "*JuceLibraryCode*" -prune -o -iname *.cc -o -iname *.cpp -o -iname *.h | \
  xargs ./cpplint.py --repository=.. --quiet --filter=$FILTER_ARG_UI --linelength=120
