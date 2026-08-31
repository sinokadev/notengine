@echo off
pushd "%~dp0.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target notengine

popd
