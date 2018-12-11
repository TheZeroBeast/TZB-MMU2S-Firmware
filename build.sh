#!/bin/bash 
BUILD_ENV="1.0.0"
BUILD_GENERATOR="Eclipse CDT4 - Ninja"
SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

if [ ! -d "build-env" ]; then
    mkdir build-env || exit 1
fi
cd build-env || exit 2

if [ ! -f "MM-build-env-Linux64-$BUILD_ENV.zip" ]; then
    wget https://github.com/prusa3d/MM-build-env/releases/download/$BUILD_ENV/MM-build-env-Linux64-$BUILD_ENV.zip || exit 3
fi

if [ ! -d "../../MM-build-env-$BUILD_ENV" ]; then
    unzip MM-build-env-Linux64-$BUILD_ENV.zip -d ../../MM-build-env-$BUILD_ENV || exit 4
fi

cd ../../MM-build-env-$BUILD_ENV || exit 5
BUILD_ENV_PATH="$( pwd -P )"
export PATH=$BUILD_ENV_PATH/cmake/bin:$BUILD_ENV_PATH/ninja:$BUILD_ENV_PATH/avr/bin:$PATH

cd ..

if [ ! -d "MM-control-01-build" ]; then
    mkdir MM-control-01-build  || exit 6
fi

cd MM-control-01-build || exit 7

if [ ! -f "$BUILD_ENV.version" ]; then
    rm -r *
    cmake -G "$BUILD_GENERATOR" $SCRIPT_PATH || exit 8
    touch $BUILD_ENV.version || exit 9
fi

cmake --build . || exit 10