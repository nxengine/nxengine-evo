#!/bin/sh
# Download and extract the required external dependencies used to complete the install step
#
# This scripts expects to be executed as part of other build scripts.
# Because of this, the following things are expected about the environment:
#  * Current working directory must be at the GIT checkout of the project root directory
#  * Environment variable ${APP_ID} must contain the application's app ID
#  * File "${APP_ID}.json" must contain the Flatpak builder manifest
#
# Call as: build-scripts/utils/common.download-extern.sh
set -eu

# Check for required non-POSIX commands
if type wget >/dev/null 2>&1;
then
	DOWNLOAD_CMD="wget -O"
elif type curl >/dev/null 2>&1;
then
	DOWNLOAD_CMD="curl -Lo"
else
	echo 'Neither of commands `wget` and `curl` (HTTPS downloader) found, aborting!' >&2 && exit 1
fi
type jq        >/dev/null 2>&1 || ( echo 'Required command `jq` (JSON query) not found, aborting!'        >&2 && exit 1 )
type sha256sum >/dev/null 2>&1 || ( echo 'Required command `sha25sum` (file hasher) not found, aborting!' >&2 && exit 1 )
type unzip     >/dev/null 2>&1 || ( echo 'Required command `unzip` (ZIP extractor) not found, aborting!'  >&2 && exit 1 )


# Download, verify and extract external resources mentioned in the Flatpak builder manifest
mkdir -p extern
jq -r '.modules[0].sources[] | select(.type=="archive") | (.sha256 + " " + .url + " " + .dest)' "${APP_ID}.json" | while read sha256 url dest;
do
	origname="${url##*/}"
	filepath="extern/${sha256}.${origname#*.}"
	
	# Download and verify archive file
	test -e "${filepath}" || ${DOWNLOAD_CMD} "${filepath}" "${url}"
	
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