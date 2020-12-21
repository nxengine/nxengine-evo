if [ ${APPVEYOR_REPO_TAG} == "true" ]; then
  sudo apt install -yqq libxml2-utils jq appstream appstream-util
  build-scripts/build-appimage.sh --no-build
  appveyor PushArtifact ${OUTPUT} -DeploymentName appimagebuild
fi
