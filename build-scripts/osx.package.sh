brew install dylibbundler --HEAD

#langs
wget https://github.com/nxengine/translations/releases/download/v1.12/all.zip
unzip all.zip

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
cp ../nxextract CaveStory/
cp ../nxengine-evo NXEngine.app/Contents/MacOS/NXEngine
cd CaveStory
./nxextract
cd ..
cp -r CaveStory/data NXEngine.app/Contents/Resources
cp -r ../data NXEngine.app/Contents/Resources
cp ../platform/osx/icons.icns NXEngine.app/Contents/Resources
cp ../platform/osx/Info.plist NXEngine.app/Contents/
rm -rf CaveStory

#bundle libs
dylibbundler -b -x NXEngine.app/Contents/MacOS/NXEngine -d NXEngine.app/Contents/libs-intel -od -p @executable_path/../libs-intel

#create dmg
cd ..
#git clone https://github.com/andreyvit/create-dmg
git clone https://github.com/isage/create-dmg
cd create-dmg
./create-dmg --volname "NXEngine" --window-size 640 480 --app-drop-link 380 205 --background ../platform/osx/bg.png --icon-size 96 --icon "NXEngine.app" 110 205 "../NXEngine_Evo-$APPVEYOR_REPO_TAG_NAME-OSX.dmg" ../release/
#curl --upload-file "../NXEngine-$TRAVIS_TAG.dmg" https://transfer.sh/NXEngine.dmg

#upload
#cd ../release
#zip -r "NXEngine-$TRAVIS_TAG.zip" NXEngine.app
#curl --upload-file "NXEngine-$TRAVIS_TAG.zip" https://transfer.sh/NXEngine.zip
cd ..
ls