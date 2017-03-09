#!/bin/bash

rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_FMUS=OFF -GNinja
cmake --build . --target package
