#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

g++ -std=c++11 -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -O0 -g -I./thirdparty -I./code \
    ./code/exporter.cpp \
    -o ./build/export -lassimp \
    -Wno-implicit-fallthrough

g++ -std=c++11 -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -O0 -g -I./thirdparty -I./code \
    ./thirdparty/stb_image.c \
    ./code/importer.cpp ./code/animation.cpp ./code/gpu.c ./code/os.c \
    -o ./build/import -lm -lX11 -lGL -lassimp -lXcursor\
    -Wno-implicit-fallthrough \
    -Wno-pedantic -Wno-write-strings 
