if [ ${APPVEYOR_REPO_TAG} == "true" ]; then

  sudo apt install -yqq libxml2-utils jq appstream appstream-util
  build-scripts/build-appimage.sh --no-build

  appveyor PushArtifact "NXEngine-${APPVEYOR_REPO_TAG_NAME}-Linux-x86_64.AppImage" -DeploymentName appimagebuild
fi
