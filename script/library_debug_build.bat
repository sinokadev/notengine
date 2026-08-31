@echo off
pushd "%~dp0.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target notengine

popd
