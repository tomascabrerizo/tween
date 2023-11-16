@echo off

if not exist .\build mkdir .\build

set TARGET=exporter
set CFLAGS=/nologo /Od /Zi /EHsc
set LIBS=User32.lib assimp-vc143-mt.lib
set SOURCES=.\src\main.cpp .\src\math.cpp .\src\memory.cpp .\src\input.cpp .\src\win32_platform.cpp .\src\platform_manager.cpp .\src\memory_manager.cpp
set SOURCES=.\code\exporter.cpp
set OUT_DIR=/Fo.\build\ /Fe.\build\%TARGET% /Fm.\build\
set INC_DIR=/I.\ /I.\thirdparty\include /I.\code
set LNK_DIR=/LIBPATH:.\thirdparty\lib

cl %CFLAGS% %INC_DIR% %SOURCES% %OUT_DIR% /link %LNK_DIR% %LIBS% /SUBSYSTEM:CONSOLE
