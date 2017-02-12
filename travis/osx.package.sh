brew install dylibbundler

#prepare
mkdir release
cd release
wget http://www.cavestory.org/downloads/cavestoryen.zip
unzip cavestoryen.zip
rm cavestoryen.zip
mkdir NXEngine.app
mkdir NXEngine.app/Contents
mkdir NXEngine.app/Contents/MacOS
mkdir NXEngine.app/Contents/Resources

#prepare and copy data
cp ../bin/extract CaveStory/
cp ../bin/nx NXEngine.app/Contents/MacOS/NXEngine
cd CaveStory
./extract
cd ..
cp -r CaveStory/data NXEngine.app/Contents/Resources
cp -r CaveStory/org NXEngine.app/Contents/Resources
cp -r CaveStory/stage.dat NXEngine.app/Contents/Resources
cp -r CaveStory/wavetable.dat NXEngine.app/Contents/Resources
cp -r CaveStory/pxt NXEngine.app/Contents/Resources
cp -r ../data NXEngine.app/Contents/Resources
cp ../font.ttf NXEngine.app/Contents/Resources
cp ../smallfont.bmp NXEngine.app/Contents/Resources
cp ../sprites.sif NXEngine.app/Contents/Resources
cp ../tilekey.dat NXEngine.app/Contents/Resources
cp ../osx/icons.icns NXEngine.app/Contents/Resources
cp ../osx/Info.plist NXEngine.app/Contents/
rm -rf CaveStory
ls

#bundle libs
dylibbundler -b -x NXEngine.app/Contents/MacOS/NXEngine -d NXEngine.app/Contents/libs-intel -od -p @executable_path/../libs-intel

#create dmg
cd ..
#git clone https://github.com/andreyvit/create-dmg
git clone https://github.com/isage/create-dmg
cd create-dmg
./create-dmg --volname "NXEngine" --window-size 640 480 --app-drop-link 380 205 --background ../osx/bg.png --icon-size 96 --icon "NXEngine.app" 110 205 NXEngine.dmg ../release/
curl --upload-file NXEngine.dmg https://transfer.sh/NXEngine.dmg

#upload
cd ../release
zip -r NXEngine.zip NXEngine.app
curl --upload-file NXEngine.zip https://transfer.sh/NXEngine.zip
