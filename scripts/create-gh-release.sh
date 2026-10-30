#!/bin/bash

set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"

MODE=${1:---all}
if [[ $# -gt 1 ]]; then
  echo "Usage: $0 [--all|--rpm-only|--deb-only]" >&2
  exit 1
fi
case "$MODE" in
  --all|--rpm-only|--deb-only) ;;
  --help) echo "Usage: $0 [--all|--rpm-only|--deb-only]"; exit 0 ;;
  *) echo "Unknown option: $MODE" >&2; exit 1 ;;
esac

VERSION=$(cat VERSION)
FC_VERSION=${FC_VERSION:-44}
TITLE=${TITLE:-"Release $VERSION"}
MESSAGE=${MESSAGE:-"Release $VERSION"}

assets=()
if [[ $MODE != --deb-only ]]; then
  for package in ibus-libime ibus-libime-debuginfo ibus-libime-debugsource; do
    assets+=("tito-out/tito-$VERSION/x86_64/$package-$VERSION-1.fc$FC_VERSION.x86_64.rpm")
  done
fi
if [[ $MODE != --rpm-only ]]; then
  read -r _ DEB_VERSION _ < debian/changelog
  DEB_VERSION=${DEB_VERSION#(}
  DEB_VERSION=${DEB_VERSION%)}
  if [[ ${DEB_VERSION%-*} != "$VERSION" ]]; then
    echo "debian/changelog version $DEB_VERSION does not match VERSION ($VERSION)." >&2
    exit 1
  fi
  for distribution in ubuntu24.04 ubuntu26.04 debian13; do
    dir="deb-out/$VERSION/$distribution"
    assets+=("$dir/ibus-libime_${DEB_VERSION}+${distribution}_amd64.deb")
    for extension in deb ddeb; do
      debug="$dir/ibus-libime-dbgsym_${DEB_VERSION}+${distribution}_amd64.$extension"
      if [[ -f $debug ]]; then
        assets+=("$debug")
      fi
    done
  done
fi

for asset in "${assets[@]}"; do
  if [[ ! -r $asset || ! -s $asset ]]; then
    echo "Missing release artifact: $asset" >&2
    echo "Build/download the artifacts first, or select --rpm-only / --deb-only." >&2
    exit 1
  fi
done

gh release create "$VERSION" "${assets[@]}" --title "$TITLE" --notes "$MESSAGE"
