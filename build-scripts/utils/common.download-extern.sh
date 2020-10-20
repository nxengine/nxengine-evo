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