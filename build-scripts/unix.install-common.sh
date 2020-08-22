#!/bin/bash
# Expects to be called during the Flatpak or AppImage build process
#
# Because of this, the following things are expected about the environment:
#  * Current working directory must be at the GIT checkout of the project root directory
#  * Directory `extern/Translations` must contain the preprocessed and extracted translations (one subdirectory for each language)
#  * Directory `extern/CaveStory` must contain the extracted Windows game binary
#  * Directory `bin` must contain the built `nx` and `extract` executables
set -eu -o pipefail
DEST="${1}"

# Extract resources from original CaveStory EXE
( cd extern/CaveStory && ../../bin/extract; )

# Install NXEngine data files
install -vd "${DEST}/share/nxengine"
cp -avr data                  "${DEST}/share/nxengine/"
cp -avr extern/CaveStory/data "${DEST}/share/nxengine/"
cp -avr extern/Translations   "${DEST}/share/nxengine/data/lang"

# Install NXEngine binary and system integration files for Unix/XDG platforms
install -vD bin/nx "${DEST}/bin/nxengine-evo"
install -vDt"${DEST}/share/applications"               platform/xdg/org.nxengine.nxengine_evo.desktop
install -vDt"${DEST}/share/icons/hicolor/256x256/apps" platform/xdg/org.nxengine.nxengine_evo.png
install -vDt"${DEST}/share/metainfo"                   platform/xdg/org.nxengine.nxengine_evo.appdata.xml
