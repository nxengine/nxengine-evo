#!/bin/bash
set -eu -o pipefail
cd "$(dirname "$(readlink -f "$0")")/.."  # GNU guard

export APP_ID="org.nxengine.nxengine_evo"

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

# Extract latest release from AppStream data
APP_VERSION="v$(xmllint --xpath 'string(/component/releases/release/@version)' "platform/xdg/${APP_ID}.appdata.xml")"

# override for CI
if [ "${APPVEYOR_REPO_TAG:-}" == "true" ]
then
    APP_VERSION="${APPVEYOR_REPO_TAG_NAME}"
fi

MACHINE="$(uname -m)"

# Download required dependencies
build-scripts/utils/common.download-extern.sh

# Additionally download recent version of the appimage-builder AppImage utility (no checksum verification since file regularily changes when updated to newer versions)
test -e "extern/appimage-builder.AppImage" || wget "https://github.com/AppImageCrafters/appimage-builder/releases/download/v1.1.0/appimage-builder-1.1.0-x86_64.AppImage" -O "extern/appimage-builder.AppImage"
chmod +x extern/appimage-builder.AppImage

# Build NXEngine-Evo
if ! ${SKIP_BUILD};
then
	rm -rf build
	cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DPORTABLE=ON -DCMAKE_INSTALL_PREFIX=/usr -Bbuild -H.
	ninja -Cbuild
fi

# Generate AppImage filesystem image directory
rm -rf AppDir
DESTDIR=AppDir ninja -Cbuild install
build-scripts/utils/common.install-extern.sh build/AppDir/usr/bin/ build/nxextract
cp -r build/AppDir/usr/share/nxengine/data/ build/AppDir/usr/bin/

export PATH="${PWD}/build/bin${PATH+:}${PATH:-}"

PLATFORM_SUFFIX=""
case "${MACHINE}" in
	i[3456789]86|x86|x86-32|x86_32)
		PLATFORM_SUFFIX="i386"
	;;
	x86-64|x86_64|amd64)
		PLATFORM_SUFFIX="x86_64"
	;;
	*)
		PLATFORM_SUFFIX="${MACHINE}"
	;;
esac

export APP_VERSION
export PLATFORM_SUFFIX
export OUTPUT="NXEngine-Evo-${APP_VERSION}-Linux-${PLATFORM_SUFFIX}.AppImage"
rm -f "${OUTPUT}"

extern/appimage-builder.AppImage --appdir build/AppDir/
