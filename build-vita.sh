#!/bin/bash

rm -rf build
rm -rf release
mkdir release
mkdir build
cd build
cmake ..
make extract
cd ..
rm -rf build

cd release
wget https://github.com/nxengine/translations/releases/download/v1.2/all.zip
unzip all.zip
rm all.zip
wget http://www.cavestory.org/downloads/cavestoryen.zip
unzip cavestoryen.zip
rm cavestoryen.zip
cd CaveStory
../../bin/extract
cp -r data/* ../data/
cd ..
rm -rf CaveStory
cp -r ../data/* data/
cd ..
mkdir build
cd build
cmake -DPLATFORM=vita ..
make
cd ..
rm -rf release/data