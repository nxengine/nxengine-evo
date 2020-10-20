#!/bin/sh
# Install external dependencies needed to complete the install step
#
# Expects to be called during the Flatpak or AppImage build process.
# Because of this, the following things are expected about the environment:
#  * Current working directory must be at the GIT checkout of the project root directory
#  * Directory `extern/Translations` must contain the preprocessed and extracted translations (one subdirectory for each language)
#  * Directory `extern/CaveStory` must contain the extracted Windows game binary
#
# Call as: build-scripts/unix.install-extras.sh [target-path:/usr] [/path/to/nxextract]
set -eu
TARGET="${1}"
NXEXTRACT="$(readlink -f "${2}")"

# Extract resources from original CaveStory EXE
( cd extern/CaveStory && "${NXEXTRACT}"; )

# Install NXEngine data files
install -vd "${TARGET}/share/nxengine"
cp -avr extern/CaveStory/data "${TARGET}/share/nxengine/"
cp -avr extern/Translations   "${TARGET}/share/nxengine/data/lang"