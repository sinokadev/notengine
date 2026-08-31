@echo off
pushd "%~dp0.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
echo demo build
cmake --build build --config Release --target demo
echo benchmark build
cmake --build build --config Release --target benchmark
echo scene_render build
cmake --build build --config Release --target scene_render

popd
