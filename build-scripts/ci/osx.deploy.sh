if [ ${APPVEYOR_REPO_TAG} == "true" ]; then
  # first parameter is path to appveyor-build-agent or its shell wrapper, hopefully
  AGENT_PATH=$(greadlink -f $1)

  BUILD_AGENT="$(dirname $AGENT_PATH)/appveyor-build-agent"

  # because agent version (and path) changes with updates we need to calculate actual csreq

  codesign --detached /tmp/appveyor-build-agent.sig -s - $BUILD_AGENT

  codesign -d -r- --detached /tmp/appveyor-build-agent.sig $BUILD_AGENT 2>&1 | awk -F ' => ' '/designated/{print $2}' | csreq -r- -b /tmp/appveyor-build-agent.csreq

  AGENT_CSREQ=$(xxd -p /tmp/appveyor-build-agent.csreq  | tr -d '\n')


  sqlite3 "/Users/appveyor/Library/Application Support/com.apple.TCC/TCC.db" "INSERT INTO access VALUES('kTCCServiceAppleEvents','$BUILD_AGENT',1,1,1,X'$AGENT_CSREQ',NULL,0,'com.apple.finder',X'fade0c000000002c00000001000000060000000200000010636f6d2e6170706c652e66696e64657200000003',NULL,1605970638);"

  brew install create-dmg dylibbundler jq
  build-scripts/build-macos.sh --no-build
  appveyor PushArtifact NXEngine-Evo-${APPVEYOR_REPO_TAG_NAME}-OSX.dmg -DeploymentName osxbuild
fi
