#!/bin/sh
set -e 
(
  cd "$(dirname "$0")" 
  cmake -B build -S .
  cmake --build ./build
)


exec ./build/shell "$@"
