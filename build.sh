#!/bin/bash

set -xe

mkdir -p ./dist
gcc --std=c17 -ggdb -Wall -Werror -mavx2 -o ./dist/drawing ./src/main.c -lglfw -lm
# gcc -O3 -Wall -Werror -mavx2 -o ./dist/drawing ./src/main.c -lglfw -lm
