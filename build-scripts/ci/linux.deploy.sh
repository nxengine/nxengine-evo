if [ ${APPVEYOR_REPO_TAG} == "true" ]; then

  sudo apt install -yqq libxml2-utils jq appstream appstream-util
  build-scripts/build-appimage.sh --no-build

  export APP_ID="org.nxengine.nxengine_evo"
  export VERSION="$(xmllint --xpath 'string(/component/releases/release/@version)' "platform/xdg/${APP_ID}.appdata.xml")"
  MACHINE="$(uname -m)"

  PLATFORM_SUFFIX=""
  case "${MACHINE}" in
        i[3456789]86|x86|x86-32|x86_32)
                PLATFORM_SUFFIX=32
        ;;
        x86-64|x86_64|amd64)
                PLATFORM_SUFFIX=64
        ;;
        *)
                PLATFORM_SUFFIX="-${MACHINE}"
        ;;
  esac

  appveyor PushArtifact "NXEngine-v${VERSION}-Linux${PLATFORM_SUFFIX}.AppImage" -DeploymentName appimagebuild
fi
