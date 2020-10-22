#!/bin/bash
set -eu -o pipefail
cd "$(dirname "$(readlink -f "$0")")/.."

export APP_ID="org.nxengine.nxengine_evo"
BUILD_DEST=build/NXEngine.app

SKIP_BUILD=false
if [[ $# -gt 0 ]];
then
	case "$1" in
		"--no-build")
			SKIP_BUILD=true
		;;

		*)
			echo "Usage: ${0##*/} [--no-build]"
			exit 1
		;;
	esac
fi

# Download required dependencies
build-scripts/utils/common.download-extern.sh

# Additionally download disk image generation utility
test -e "extern/create-dmg/create-dmg" || git clone https://github.com/isage/create-dmg.git extern/create-dmg

# Build NXEngine-Evo
if ! ${SKIP_BUILD};
then
	rm -rf build
	cmake -GNinja -DCMAKE_BUILD_TYPE=Release -Bbuild -H.
	ninja -Cbuild
fi

# Copy main binary
install -vT build/nxengine-evo      "${BUILD_DEST}/Contents/MacOS/NXEngine"
install -vT platform/osx/icons.icns "${BUILD_DEST}/Contents/Resources/icons.icns"
install -vT platform/osx/Info.plist "${BUILD_DEST}/Contents/Info.plist"

# Copy game data
install -vd "${BUILD_DEST}/Contents/Resources"
cp -avr data "${BUILD_DEST}/Contents/Resources/"
build-scripts/utils/common.install-extern.sh "${BUILD_DEST}/Contents/Resources" build/nxextract

# Bundle dynamic libraries
dylibbundler -b -x "${BUILD_DEST}/Contents/MacOS/NXEngine" -d "${BUILD_DEST}/Contents/libs-intel" -od -p '@executable_path/../libs-intel'

# Create disk image
extern/create-dmg/create-dmg \
	--volname "NXEngine" \
	--window-size 640 480 \
	--app-drop-link 380 205 \
	--background platform/osx/bg.png \
	--icon-size 96 \
	--icon "NXEngine.app" 110 205 \
	"NXEngine_Evo-${APPVEYOR_REPO_TAG_NAME}-OSX.dmg" "${BUILD_DEST}"

ls