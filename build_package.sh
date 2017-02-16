#!/bin/bash

rm -rf oos
mkdir -p oos
cd oos
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_FMUS=OFF -GNinja
cmake --build . --target package
