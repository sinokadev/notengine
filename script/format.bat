@echo off
pushd "%~dp0.."

for %%f in (demo\*.cpp) do clang-format -i "%%f"
for %%f in (src\*.cpp) do clang-format -i "%%f"
for %%f in (include\knot\*.h include\knot\*.hpp) do clang-format -i "%%f"
for %%f in (include\knot\utility\*.h include\knot\utility\*.hpp) do clang-format -i "%%f"

popd
