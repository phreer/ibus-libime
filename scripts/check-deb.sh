#!/bin/bash

set -euo pipefail

if [[ $# != 1 || ! -f $1 ]]; then
  echo "Usage: $0 path/to/ibus-libime_VERSION_amd64.deb" >&2
  exit 1
fi

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PROJECT_VERSION=$(cat "$ROOT/VERSION")
# shellcheck source=/dev/null
source /etc/os-release
DIST="$ID$VERSION_ID"
test "$(dpkg-deb --field "$1" Package)" = ibus-libime
test "$(dpkg-deb --field "$1" Architecture)" = amd64
PACKAGE_VERSION=$(dpkg-deb --field "$1" Version)
case "$PACKAGE_VERSION" in
  "$PROJECT_VERSION"-*+"$DIST") ;;
  *) echo "Unexpected package version: $PACKAGE_VERSION (expected $PROJECT_VERSION for $DIST)." >&2; exit 1 ;;
esac
dpkg-deb --info "$1"
DEPENDS=$(dpkg-deb --field "$1" Depends)
for dependency in ibus libime-data libime-data-language-model; do
  if ! grep -Eq "(^|, )$dependency([ ,(]|$)" <<< "$DEPENDS"; then
    echo "Missing runtime dependency: $dependency" >&2
    exit 1
  fi
done

WORK=$(mktemp -d --tmpdir ibus-libime-check.XXXXXXXX)
trap 'rm -rf -- "$WORK"' EXIT
dpkg-deb --extract "$1" "$WORK"
for program in ibus-engine-libime ibus-setup-libime; do
  test -x "$WORK/usr/libexec/$program"
done
grep -Fx '    <exec>/usr/libexec/ibus-engine-libime</exec>' "$WORK/usr/share/ibus/component/libime.xml"
grep -Fx '            <setup>/usr/libexec/ibus-setup-libime</setup>' "$WORK/usr/share/ibus/component/libime.xml"
grep -Fx 'Exec=/usr/libexec/ibus-setup-libime' "$WORK/usr/share/applications/ibus-setup-libime.desktop"
for data in english-frequency.txt english-technical.txt english-README.txt english-LICENSE.txt; do
  test -s "$WORK/usr/share/ibus-libime/$data"
done
test -s "$WORK/usr/share/doc/ibus-libime/copyright"
