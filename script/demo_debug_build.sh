#!/bin/bash

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
echo demo build
cmake --build build --target demo
echo benchmark build
cmake --build build --target benchmark
echo scene_render build
cmake --build build --target scene_render