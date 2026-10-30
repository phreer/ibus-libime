#!/bin/bash

set -euo pipefail

unset FC_VERSION TITLE MESSAGE
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
WORK=$(mktemp -d --tmpdir ibus-libime-release-test.XXXXXXXX)
trap 'rm -rf -- "$WORK"' EXIT
mkdir -p "$WORK/project/scripts" "$WORK/project/debian" "$WORK/bin"
cp "$ROOT/scripts/create-gh-release.sh" "$WORK/project/scripts/"
cp "$ROOT/VERSION" "$WORK/project/"
cp "$ROOT/debian/changelog" "$WORK/project/debian/"
export GH_CALL_LOG="$WORK/gh-args"
export PATH="$WORK/bin:$PATH"
cat > "$WORK/bin/gh" <<'EOF'
#!/bin/bash
printf '%s\n' "$@" > "$GH_CALL_LOG"
EOF
chmod +x "$WORK/bin/gh"
cd "$WORK/project"
VERSION=$(cat VERSION)
read -r _ DEB_VERSION _ < debian/changelog
DEB_VERSION=${DEB_VERSION#(}
DEB_VERSION=${DEB_VERSION%)}

expect_failure() {
  rm -f "$GH_CALL_LOG"
  if ./scripts/create-gh-release.sh "$@" > "$WORK/output" 2>&1; then
    echo "Expected failure: $*" >&2
    exit 1
  fi
  test ! -e "$GH_CALL_LOG"
}

expect_failure
grep -q 'Missing release artifact:' "$WORK/output"
expect_failure --unknown
expect_failure --all unexpected

rpm_dir="tito-out/tito-$VERSION/x86_64"
mkdir -p "$rpm_dir"
for package in ibus-libime ibus-libime-debuginfo ibus-libime-debugsource; do
  printf 'fixture\n' > "$rpm_dir/$package-$VERSION-1.fc44.x86_64.rpm"
done
./scripts/create-gh-release.sh --rpm-only
test "$(grep -c '\.rpm$' "$GH_CALL_LOG")" -eq 3
! grep -q '\.deb$' "$GH_CALL_LOG"

for distribution in ubuntu24.04 ubuntu26.04 debian13; do
  dir="deb-out/$VERSION/$distribution"
  mkdir -p "$dir"
  printf 'fixture\n' > "$dir/ibus-libime_${DEB_VERSION}+${distribution}_amd64.deb"
done
./scripts/create-gh-release.sh --deb-only
test "$(grep -c '\.deb$' "$GH_CALL_LOG")" -eq 3
! grep -q '\.rpm$' "$GH_CALL_LOG"
printf 'fixture\n' > "deb-out/$VERSION/debian13/ibus-libime-dbgsym_${DEB_VERSION}+debian13_amd64.deb"
printf 'fixture\n' > "deb-out/$VERSION/ubuntu24.04/ibus-libime-dbgsym_${DEB_VERSION}+ubuntu24.04_amd64.ddeb"
TITLE='Custom title' MESSAGE='Custom notes' ./scripts/create-gh-release.sh
test "$(grep -c '\.deb$' "$GH_CALL_LOG")" -eq 4
test "$(grep -c '\.ddeb$' "$GH_CALL_LOG")" -eq 1
test "$(grep -c '\.rpm$' "$GH_CALL_LOG")" -eq 3
grep -Fx "$VERSION" "$GH_CALL_LOG"
grep -Fx 'Custom title' "$GH_CALL_LOG"
grep -Fx 'Custom notes' "$GH_CALL_LOG"

rm "deb-out/$VERSION/debian13/ibus-libime_${DEB_VERSION}+debian13_amd64.deb"
expect_failure
grep -q 'debian13/ibus-libime_' "$WORK/output"
printf '9.9.9\n' > VERSION
expect_failure --deb-only
grep -q 'does not match VERSION' "$WORK/output"

echo "Release script tests passed."
