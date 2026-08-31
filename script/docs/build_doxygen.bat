@echo off
pushd "%~dp0..\..\docs"

doxygen -g Doxyfile

popd
