#!/bin/bash

cmake -S . -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --target notengine