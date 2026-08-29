#!/bin/bash
set -x
set -e
cd /home/michen/dev/rv64-vm
CXX=g++
SRCS=$(find src -name '*.cpp')
SRCS=$(echo "$SRCS" | grep -v 'src/misc/xdg-shell-protocol.cpp')
CXXFLAGS="-std=gnu++20 -O1 -g -fno-omit-frame-pointer -fsanitize=thread -march=native -mbmi2 -Iinclude -include unistd.h -include termios.h -DUSE_THREADED_HARTS=1 -DUSE_BLOCK_JIT=1 -DUSE_FPU=1"
LIBS="-latomic -pthread"
rm -rf build.tsan
mkdir -p build.tsan/obj
OBJS=""
for s in $SRCS; do
  o="build.tsan/obj/$(echo $s | tr '/' '_').o"
  $CXX $CXXFLAGS -c "$s" -o "$o" 2>build.tsan/err.log || { echo "COMPILE FAIL: $s"; tail -5 build.tsan/err.log; exit 1; }
  OBJS="$OBJS $o"
done
$CXX -fsanitize=thread $OBJS -o build.tsan/tsan_bin $LIBS
echo "BUILT build.tsan/tsan_bin"
