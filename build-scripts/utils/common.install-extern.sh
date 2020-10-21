#!/bin/sh
# Install external dependencies needed to complete the install step
#
# Expects to be called during the Flatpak or AppImage build process.
# Because of this, the following things are expected about the environment:
#  * Current working directory must be at the GIT checkout of the project root directory
#  * Directory `extern/Translations` must contain the preprocessed and extracted translations (one subdirectory for each language)
#  * Directory `extern/CaveStory` must contain the extracted Windows game ZIP
#
# Call as: build-scripts/unix.install-extern.sh [target-path:/usr/share/nxengine] [/path/to/nxextract]
set -eu
TARGET="${1}"
NXEXTRACT="`case "$2" in /*) echo "$2";; *) echo "$(pwd)/$2";; esac`"

# Extract resources from original CaveStory EXE
( cd extern/CaveStory && "${NXEXTRACT}"; )

# Install NXEngine data files
install -vd "${TARGET}"
cp -v -RpP extern/CaveStory/data "${TARGET}/"
cp -v -RpP extern/Translations   "${TARGET}/data/lang"