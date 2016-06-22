@echo off

mkdir ..\..\build
pushd ..\..\build
cl -FC -Zi ..\engine\src\win32_engine.cpp user32.lib Gdi32.lib
popd
