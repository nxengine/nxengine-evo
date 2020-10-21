#!/bin/sh
set -eu
cd "$(dirname "$(readlink -f "$0")")/.."  # GNU guard

APP_ID="org.nxengine.nxengine_evo"
VERSION="$(xmllint --xpath 'string(/component/releases/release/@version)' platform/xdg/org.nxengine.nxengine_evo.appdata.xml)"

# Build Flatpak and export it into build/flatpak-repo
flatpak-builder --ccache --force-clean --repo=build/flatpak-repo "$@" build/flatpak-build "${APP_ID}.json"

# Export Flatpak bundles from repo
flatpak build-bundle build/flatpak-repo           "NXEngine-v${VERSION}-Linux64.flatpak"       "${APP_ID}"
flatpak build-bundle build/flatpak-repo --runtime "NXEngine-v${VERSION}-Linux64.Debug.flatpak" "${APP_ID}.Debug"