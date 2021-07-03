if($env:APPVEYOR_REPO_TAG -eq "true") {
  Invoke-WebRequest "https://github.com/nxengine/translations/releases/download/v1.14/all.zip" -Out all.zip
  7z x .\all.zip
  rm .\all.zip

  mkdir release
  cd release
  wget https://sarcasticat.com/cavestoryen.zip -outfile cavestoryen.zip
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
    cp ..\platform\win32\ext\runtime\x86\* .\NXEngine\

    #create zip
    $name = "NXEngine-Evo-$env:APPVEYOR_REPO_TAG_NAME-Win32.zip"
    7z a $name .\NXEngine\

    Push-AppveyorArtifact $name -DeploymentName winbuild

  } else {
    #bundle libs
    cp ..\platform\\win32\ext\runtime\x64\* .\NXEngine\

    #create zip
    $name = "NXEngine-Evo-$env:APPVEYOR_REPO_TAG_NAME-Win64.zip"
    7z a $name .\NXEngine\

    Push-AppveyorArtifact $name -DeploymentName winbuild
  }
}