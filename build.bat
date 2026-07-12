@echo off

pushd bin
cl /Zi /nologo /I../include ../src/*.c ../src/renderer/*.c /Fe:game.exe /link ../lib/raylib_debug.lib winmm.lib opengl32.lib gdi32.lib user32.lib shell32.lib /NODEFAULTLIB:LIBCMTD
popd