if [ ${APPVEYOR_REPO_TAG} == "true" ]; then
  brew install create-dmg dylibbundler jq
  build-scripts/build-macos.sh --no-build
  appveyor PushArtifact NXEngine-Evo-${APPVEYOR_REPO_TAG_NAME}-OSX.dmg -DeploymentName osxbuild

  sqlite3 "$HOME/Library/Application Support/com.apple.TCC/TCC.db" ".dump access;"
fi
