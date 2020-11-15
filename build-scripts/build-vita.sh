#!/bin/bash

export APP_ID="org.nxengine.nxengine_evo"
build-scripts/utils/common.download-extern.sh

rm -rf build
#rm -rf release
mkdir -p release
mkdir -p build
cd build
cmake ..
make extract
cd ..

build-scripts/utils/common.install-extern.sh release/ build/nxextract

cp -v -RpP data "release/"

rm -rf build
mkdir build
cd build
cmake -DPLATFORM=vita -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
#rm -rf release/data