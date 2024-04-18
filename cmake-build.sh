#!/bin/sh

set -e

mkdir -p ./libmikmod/build ./mikmod/build

## build libmikmod library first:
cmake -S ./libmikmod -B libmikmod/build -DCMAKE_BUILD_TYPE=Release
cmake --build ./libmikmod/build/ --config Release

## now build mikmod player against it:
CMAKE_PREFIX_PATH=./libmikmod:./libmikmod/build cmake -S mikmod -B mikmod/build -DCMAKE_BUILD_TYPE=Release
cmake --build mikmod/build/ --config Release
