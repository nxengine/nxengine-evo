#prepare
mkdir release
cd release
wget http://www.cavestory.org/downloads/cavestoryen.zip -outfile cavestoryen.zip
7z x .\cavestoryen.zip
rm .\cavestoryen.zip
mkdir NXEngine

Write-Host $env:PLATFORM

#prepare and copy data
if ($env:PLATFORM -eq "Win32") {
  cp ..\bin\Win32\Release\extract_Win32.exe .\CaveStory\
  cp ..\bin\Win32\Release\nx_Win32.exe .\NXEngine\
  cd .\CaveStory\
  .\extract_Win32.exe
  cd ..
} else {
  cp ..\bin\x64\Release\extract_x64.exe .\CaveStory\
  cp ..\bin\x64\Release\nx_x64.exe .\NXEngine\
  cd .\CaveStory\
  .\extract_x64.exe
  cd ..
}

cp -r .\CaveStory\data\ .\NXEngine\
cp -r .\CaveStory\org\ .\NXEngine\
cp -r .\CaveStory\pxt\ .\NXEngine\
cp .\CaveStory\stage.dat .\NXEngine\
cp .\CaveStory\wavetable.dat .\NXEngine\
cp -r -force ..\data\ .\NXEngine\
cp ..\font.ttf .\NXEngine\
cp ..\smallfont.bmp .\NXEngine\
cp ..\sprites.sif .\NXEngine\
cp ..\tilekey.dat .\NXEngine\


if ($env:PLATFORM -eq "Win32") {
  #bundle libs
  cp -r ..\win32\ext\runtime\x86 .\NXEngine\

  #create zip
  7z a NXEngine-windows-Win32.zip .\NXEngine\

  #upload
  invoke-webrequest -UseBasicParsing -method put -infile .\NXEngine-windows-Win32.zip https://transfer.sh/NXEngine-windows-Win32.zip
} else {
  #bundle libs
  cp -r ..\win32\ext\runtime\x64 .\NXEngine\

  #create zip
  7z a NXEngine-windows-Win64.zip .\NXEngine\

  #upload
  invoke-webrequest -UseBasicParsing -method put -infile .\NXEngine-windows-Win64.zip https://transfer.sh/NXEngine-windows-Win64.zip

}