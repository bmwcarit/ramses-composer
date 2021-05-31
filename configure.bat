@echo off
echo Checking out submodules
git submodule update
echo Generating solution file to use with Visual Studio 2019
cmake -S . -B builds -G "Visual Studio 16 2019" -A x64
