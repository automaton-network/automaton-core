#!/bin/bash
function join_by { local IFS="$1"; shift; echo "$*"; }

FILTERS=(
  "-legal/copyright"
  "-runtime/explicit"
  "-build/c++11"
)

FILTER_ARG=$(join_by "," "${FILTERS[@]}")

find . -path "./playground" -prune -o -iname *.cc -o -iname *.cpp -o -iname *.h | \
  xargs ./cpplint.py --repository=.. --quiet --filter=$FILTER_ARG --linelength=120
