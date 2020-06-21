#!/bin/bash

DEBUG=1
if [ $DEBUG == "1" ]
then
    echo "*"
    echo "Compiling in Debug mode"
    echo "*"
    clang++ -D DEBUG -g -lglew -lglfw -framework OpenGL main.c -o build/build.out
else
    clang++ -g -lglew -lglfw -framework OpenGL main.c -o build/build.out
fi

