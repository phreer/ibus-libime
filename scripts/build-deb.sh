#!/bin/bash

set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"

if [[ ! -r /etc/os-release ]]; then
  echo "Cannot identify the build distribution." >&2
  exit 1
fi
# shellcheck source=/dev/null
source /etc/os-release
case "$ID:$VERSION_ID" in
  ubuntu:24.04|ubuntu:26.04|debian:13) DIST="$ID$VERSION_ID" ;;
  *)
    echo "Build in Ubuntu 24.04, Ubuntu 26.04 or Debian 13 (not $ID $VERSION_ID)." >&2
    exit 1
    ;;
esac

for tool in dpkg-buildpackage dpkg-checkbuilddeps dpkg-parsechangelog dch; do
  if ! command -v "$tool" >/dev/null; then
    echo "Missing $tool; install dpkg-dev and devscripts (see docs/development.md)." >&2
    exit 1
  fi
done
if [[ $(dpkg --print-architecture) != amd64 ]]; then
  echo "The supported build architecture is amd64." >&2
  exit 1
fi
VERSION=$(cat VERSION)
DEB_VERSION=$(dpkg-parsechangelog --show-field Version)
if [[ ${DEB_VERSION%-*} != "$VERSION" ]]; then
  echo "debian/changelog version $DEB_VERSION does not match VERSION ($VERSION)." >&2
  exit 1
fi
dpkg-checkbuilddeps

OUTPUT="$ROOT/deb-out/$VERSION/$DIST"
WORK=$(mktemp -d --tmpdir ibus-libime-deb.XXXXXXXX)
trap 'rm -rf -- "$WORK"' EXIT
mkdir "$WORK/source"
cp -a CMakeLists.txt VERSION config.h.in LICENSE README.md data src tests debian \
  "$WORK/source/"
cd "$WORK/source"
dch --newversion "$DEB_VERSION+$DIST" --distribution "$VERSION_CODENAME" \
  --force-distribution --maintmaint "Build for $PRETTY_NAME."
dpkg-buildpackage --build=binary --no-sign

shopt -s nullglob
artifacts=("$WORK"/*.deb "$WORK"/*.ddeb "$WORK"/*.changes "$WORK"/*.buildinfo)
mkdir -p "$OUTPUT"
cp -- "${artifacts[@]}" "$OUTPUT/"
echo "Debian build artifacts: $OUTPUT"
