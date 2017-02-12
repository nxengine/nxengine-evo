brew install dylibbundler

#prepare
mkdir release
cd release
wget http://www.cavestory.org/downloads/cavestoryen.zip
unzip cavestoryen.zip
mkdir NXEngine.app
mkdir NXEngine.app/Contents
mkdir NXEngine.app/Contents/MacOS
mkdir NXEngine.app/Contents/Resources

#prepare and copy data
cp ../bin/extract CaveStory/Doukutsu.exe
cp ../bin/nx NXEngine.app/Contents/MacOS/NXEngine
cd CaveStory
./extract
cd ..
cp -r CaveStory/data NXEngine.app/Contents/Resources
cp -r ../data NXEngine.app/Contents/Resources
cp ../font.ttf NXEngine.app/Contents/Resources
cp ../smallfont.bmp NXEngine.app/Contents/Resources
cp ../sprites.sif NXEngine.app/Contents/Resources
cp ../tilekey.dat NXEngine.app/Contents/Resources

#bundle libs
dylibbundler -b -x NXEngine.app/Contents/MacOS/NXEngine -d NXEngine.app/Contents/libs-intel -od -p @executable_path/../libs-intel

#create dmg

#upload
zip -r NXEngine.zip NXEngine.app
curl --upload-file NXEngine.zip https://transfer.sh/nxengine
