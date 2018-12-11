#!/bin/bash
git ls-files | grep -v 'third_party' | xargs -n1 git blame --line-porcelain | sed -n 's/^author //p' | sort -f | uniq -ic | sort -nr
