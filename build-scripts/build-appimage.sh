#!/bin/bash
set -eu -o pipefail
cd "$(dirname "$(readlink -f "$0")")/.."

export APP_ID="org.nxengine.nxengine_evo"

# Extract latest release from AppStream data
VERSION="$(xmllint --xpath 'string(/component/releases/release/@version)' "platform/xdg/${APP_ID}.appdata.xml")"
MACHINE="$(uname -m)"

# Download required dependencies
build-scripts/utils/common.download-extern.sh

# Additionally download recent version of the LinuxDeploy AppImage utility (no checksum verification since file regularily changes when updated to newer versions)
test -e "extern/linuxdeploy.AppImage" || wget "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${MACHINE}.AppImage" -O "extern/linuxdeploy.AppImage"
chmod +x extern/linuxdeploy.AppImage

# Build NXEngine-Evo
rm -rf build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=build/AppDir/usr -Bbuild -H.
ninja -Cbuild

# Generate AppImage filesystem image directory
ninja -Cbuild install
build-scripts/utils/unix.install-extern.sh build/AppDir/usr build/nxextract

# Work around GH/AppImage/AppImageKit#856
mkdir -p build/bin
for toolname in appstreamcli appstream-util;
do
	# START OF INLINE WORKAROUND SCRIPT #
	cat >"build/bin/${toolname}" <<'EOF'
#!/bin/bash
set -eu -o pipefail
SCRIPTDIR="${0%/*}"

# Remove current directory of script from search path
OLDIFS="${IFS}"
OLDPATH="${PATH}"
IFS=":"
PATH=""
for path in ${OLDPATH};
do
	if [[ ${path} != ${SCRIPTDIR} ]];
	then
		PATH+="${PATH+:}${path}"
	fi
done
IFS="${OLDIFS}"
export PATH

# Re-exec to real tool without any LD_LIBRARY_PATH set (works around GH/AppImage/AppImageKit#856)
unset LD_LIBRARY_PATH
exec "${0##*/}" "$@"
EOF
	# END OF INLINE WORKAROUND SCRIPT #
	chmod +x "build/bin/${toolname}"
done
export PATH="${PWD}/build/bin${PATH+:}${PATH:-}"

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

export VERSION
export OUTPUT="NXEngine-v${VERSION}-Linux${PLATFORM_SUFFIX}.AppImage"
rm -f "${OUTPUT}"
extern/linuxdeploy.AppImage \
	--appdir=build/AppDir \
	--desktop-file="platform/xdg/${APP_ID}.desktop" \
	--icon-file="platform/xdg/${APP_ID}.png" \
	--output=appimage