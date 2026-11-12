#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export PATH="/workspace/tools/build/bin:$PATH"
export PKG_CONFIG_SYSROOT_DIR=/workspace/tools/sysroot
export PKG_CONFIG_LIBDIR=/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/tools/sysroot/usr/share/pkgconfig
export CXXFLAGS='-I/workspace/tools/sysroot/usr/include -I/workspace/tools/sysroot/usr/include/x86_64-linux-gnu'
export LDFLAGS='-L/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu -Wl,-rpath,/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu'
configuration="${1:-Release}"
case "$configuration" in Release|Debug) ;; *) echo 'Use Release or Debug' >&2; exit 1;; esac
build="build-${configuration,,}"
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE="$configuration" -DFETCHCONTENT_SOURCE_DIR_JUCE=/workspace/deps/JUCE -DBUILD_TESTING=ON
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --output-on-failure
