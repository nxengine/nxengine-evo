#!/bin/bash

#prepare and build
rm -rf build
rm -rf release
mkdir release
mkdir build
cd build
cmake ..
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..

#prepare data
cd release
mkdir AppDir
cd AppDir
mkdir -p usr/bin
mkdir -p usr/share/nxengine/data/
mkdir -p usr/share/metainfo/
wget https://github.com/nxengine/translations/releases/download/v1.8/all.zip
unzip all.zip
rm all.zip
wget http://www.cavestory.org/downloads/cavestoryen.zip
unzip cavestoryen.zip
rm cavestoryen.zip
cd CaveStory
../../../bin/extract
cp -r data/* ../usr/share/nxengine/data/
cd ..
rm -rf CaveStory
cp -r ./data/* ./usr/share/nxengine/data/
rm -rf ./data
cp -r ../../data/* ./usr/share/nxengine/data/
cp ../../bin/nx usr/bin/nxengine-evo
cp ../../platform/appimage/nxengine-evo.appdata.xml usr/share/metainfo/nxengine-evo.appdata.xml
cd ..
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# appstream in appimagetool is broken, and looks like no one cares. Sooo..
# Use OUTPUT HACK: appimage plugin is stupid and just appends whatever's in OUTPUT to appimagetool command line. Use it to disable appstream check
OUTPUT=-n VERSION=2.6.4 ./linuxdeploy-x86_64.AppImage --appdir AppDir -l/lib64/libvorbisfile.so.3 -i ../platform/appimage/nxengine-evo.png -d ../platform/appimage/nxengine-evo.desktop --output appimage
rm -rf ./AppDir
rm ./linuxdeploy-x86_64.AppImage
cd ..
