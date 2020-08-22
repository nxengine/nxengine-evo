#!/bin/bash
set -eu -o pipefail
cd "$(dirname "$(readlink -f "$0")")/.."

APP_ID="org.nxengine.nxengine_evo"
EXT_TL_VERSION="1.9"
EXT_TL_SHA256SUM="9661ef3a8a051d0cc94f6846c8b746b9a42eb8e71fb8deedb1fe43bef5bde869"
EXT_CS_SHA256SUM="aa87fa30bee9b4980640c7e104791354e0f1f6411ee0d45a70af70046aa0685f"

# Extract latest release from AppStream data
VERSION="$(xmllint --xpath 'string(/component/releases/release/@version)' "platform/xdg/${APP_ID}.appdata.xml")"
MACHINE="$(uname -m)"

# Download, verify and extract external resources
mkdir -p extern
rm -rf extern/{CaveStory,Translations}
test -e "extern/translations-${EXT_TL_VERSION}.zip" || wget "https://github.com/nxengine/translations/releases/download/v${EXT_TL_VERSION}/all.zip" -O "extern/translations-${EXT_TL_VERSION}.zip"
test -e "extern/cavestory.zip"                      || wget "https://cavestory.org/downloads/cavestoryen.zip"                                       -O "extern/cavestory.zip"
test -e "extern/linuxdeploy.AppImage"               || wget "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${MACHINE}.AppImage" -O "extern/linuxdeploy.AppImage"

# (There is no checksum for LinuxDeploy here, as it does not publish stable releases.)
sha256sum -c <<EOF
${EXT_TL_SHA256SUM}  extern/translations-${EXT_TL_VERSION}.zip
${EXT_CS_SHA256SUM}  extern/cavestory.zip
EOF

unzip -d extern/Translations "extern/translations-${EXT_TL_VERSION}.zip"
mv extern/Translations/data/lang/* extern/Translations/
rmdir extern/Translations/data/lang extern/Translations/data

unzip -d extern "extern/cavestory.zip"

chmod +x extern/linuxdeploy.AppImage

# Build NXEngine-Evo
rm -rf build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -Bbuild -H.
ninja -Cbuild

# Generate AppImage filesystem image directory
mkdir -p build/AppDir/usr
build-scripts/unix.install-common.sh build/AppDir/usr

# Work around GH/AppImage/AppImageKit#856
mkdir -p build/bin
for toolname in appstreamcli appstream-util;
do
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