#!/bin/bash

DEBUG=0
if [ $DEBUG == "1" ]
then
    echo "*"
    echo "Compiling in Debug mode"
    echo "*"
    clang++ -D DEBUG -g -lfreetype -lglew -lglfw -I/usr/local/opt/freetype/include/freetype2 -framework OpenGL main.c -o build/build.out
else
    clang++ -O2 -g -lfreetype -lglew -lglfw -I/usr/local/opt/freetype/include/freetype2 -framework OpenGL main.c -o build/build.out
fi

