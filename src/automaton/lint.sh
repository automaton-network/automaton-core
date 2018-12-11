#!/bin/bash
function join_by { local IFS="$1"; shift; echo "$*"; }

FILTERS=(
  "-legal/copyright"
#  "-build/header_guard"
  "-build/c++11"
#  "-build/include_order"
)

FILTER_ARG=$(join_by "," "${FILTERS[@]}")

find . -path "./bazel-*" -prune -o -iname *.cc -o -iname *.cpp -o -iname *.h | \
  xargs ./cpplint.py --repository=.. --quiet --filter=$FILTER_ARG --linelength=120
