#prepare

$tag = (Invoke-WebRequest "https://api.github.com/repos/nxengine/nxengine-evo/releases" | ConvertFrom-Json)[0].tag_name

Invoke-WebRequest "https://github.com/nxengine/nxengine-evo/releases/download/$tag/all.zip" -Out all.zip
Expand-Archive all.zip -Force

mkdir release
cd release
wget http://www.cavestory.org/downloads/cavestoryen.zip -outfile cavestoryen.zip
7z x .\cavestoryen.zip
rm .\cavestoryen.zip
mkdir NXEngine

#prepare and copy data
if ($env:PLATFORM -eq "x86") {
  cp ..\bin\Win32\Release\extract_Win32.exe .\CaveStory\extract.exe
  cp ..\bin\Win32\Release\nx_Win32.exe .\NXEngine\nx.exe
} else {
  cp ..\bin\x64\Release\extract_x64.exe .\CaveStory\extract.exe
  cp ..\bin\x64\Release\nx_x64.exe .\NXEngine\nx.exe
}

cd .\CaveStory\
.\extract.exe
cd ..

cp -r .\CaveStory\data\ .\NXEngine\
cp -r -force ..\data\ .\NXEngine\

if ($env:PLATFORM -eq "x86") {
  #bundle libs
  cp ..\win32\ext\runtime\x86\* .\NXEngine\

  #create zip
  $name = "NXEngine-$env:APPVEYOR_REPO_TAG_NAME-Win32.zip"
  7z a $name .\NXEngine\

  Push-AppveyorArtifact $name

  #upload
  #invoke-webrequest -UseBasicParsing -method put -infile ".\$name" https://transfer.sh/$name
} else {
  #bundle libs
  cp ..\win32\ext\runtime\x64\* .\NXEngine\

  #create zip
  $name = "NXEngine-$env:APPVEYOR_REPO_TAG_NAME-Win64.zip"
  7z a $name .\NXEngine\

  Push-AppveyorArtifact $name

  #upload
  #invoke-webrequest -UseBasicParsing -method put -infile ".\$name" https://transfer.sh/$name

}