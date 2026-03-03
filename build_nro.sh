#!/bin/bash
set -e

# Build the project
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DPLATFORM_SWITCH=ON
make
