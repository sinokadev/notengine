#!/bin/bash

clang-format -i demo/*.cpp
clang-format -i src/*.cpp
clang-format -i include/knot/*
clang-format -i include/knot/utility/*