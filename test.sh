#!/bin/bash
cd ..
if [ ! -d "MM-control-01-test" ]; then
    mkdir MM-control-01-test || exit 1
fi

cd MM-control-01-test || exit 2
rm -r *
cmake -G "Eclipse CDT4 - Ninja" ../MM-control-01/Tests || exit 3
cmake --build . || exit 4
./tests || exit 5
