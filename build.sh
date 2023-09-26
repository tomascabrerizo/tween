#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

g++ -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -O0 -g -I./thirdparty -I./code \
    ./code/main.cpp  \
    -o ./build/tween -lm -lX11 -lGL -lassimp -lXcursor\
    -Wno-implicit-fallthrough
