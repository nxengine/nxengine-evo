if [ ${APPVEYOR_REPO_TAG} == "true" ]; then
  sqlite3 "$HOME/Library/Application Support/com.apple.TCC/TCC.db" "INSERT INTO access VALUES('kTCCServiceAppleEvents','$(dirname "$0")/appveyor-build-agent',1,1,1,X'fade0c000000002800000001000000080000001469862bbf79a6795b784518beed2ae09bf63e58ef',NULL,0,'com.apple.finder',X'fade0c000000002c00000001000000060000000200000010636f6d2e6170706c652e66696e64657200000003',NULL,1605970638);"

  brew install create-dmg dylibbundler jq
  build-scripts/build-macos.sh --no-build
  appveyor PushArtifact NXEngine-Evo-${APPVEYOR_REPO_TAG_NAME}-OSX.dmg -DeploymentName osxbuild
fi
