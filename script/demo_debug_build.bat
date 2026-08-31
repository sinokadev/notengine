@echo off
pushd "%~dp0.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
echo demo build
cmake --build build --config Debug --target demo
echo benchmark build
cmake --build build --config Debug --target benchmark
echo scene_render build
cmake --build build --config Debug --target scene_render

popd
