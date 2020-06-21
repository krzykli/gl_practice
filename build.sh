#!/bin/bash

DEBUG=0
if [ $DEBUG == "1" ]
then
    echo "*"
    echo "Compiling in Debug mode"
    echo "*"
    clang++ -D DEBUG -g -lglew -lglfw -framework OpenGL main.c -o build/build.out
else
    clang++ -O2 -g -lglew -lglfw -framework OpenGL main.c -o build/build.out
fi

