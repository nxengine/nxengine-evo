#!/bin/bash
set -eu -o pipefail
cd "$(dirname "$(readlink -f "$0")")/.."

APP_ID="org.nxengine.nxengine_evo"

# Extract latest release from AppStream data
VERSION="$(xmllint --xpath 'string(/component/releases/release/@version)' "platform/xdg/${APP_ID}.appdata.xml")"
MACHINE="$(uname -m)"

# Download, verify and extract external resources mentioned in the Flatpak builder manifest
mkdir -p extern
jq -r '.modules[0].sources[] | select(.type=="archive") | (.sha256 + " " + .url + " " + .dest)' "${APP_ID}.json" | while read sha256 url dest;
do
	origname="${url##*/}"
	filepath="extern/${sha256}.${origname#*.}"
	
	# Download and verify archive file
	test -e "${filepath}" || wget "${url}" -O "${filepath}"
	echo "${sha256}  ${filepath}" | sha256sum -c
	
	# Extract archive and move it to the same location that it would end up in the Flatpak build
	case "${dest}" in
		extern/Translations)
			rm -rf extern/Translations
			
			unzip -d extern/Translations "${filepath}"
			mv extern/Translations/data/lang/* extern/Translations/
			rmdir extern/Translations/data/lang extern/Translations/data
		;;
		
		extern/CaveStory)
			rm -rf extern/CaveStory
			
			unzip -d extern "${filepath}"
		;;
	esac
done

# Additionally download recent version of the LinuxDeploy AppImage utility (no checksum verification since file regularily changes when updated to newer versions)
test -e "extern/linuxdeploy.AppImage" || wget "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${MACHINE}.AppImage" -O "extern/linuxdeploy.AppImage"
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