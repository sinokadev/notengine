#!/bin/bash

cmake -S . -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target demo