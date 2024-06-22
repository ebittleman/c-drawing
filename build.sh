#!/bin/bash

set -xe

mkdir -p ./dist
gcc -ggdb -Wall -Werror -mavx2 -o dist/drawing main.c -lglfw
