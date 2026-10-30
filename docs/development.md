# RPM 打包

```bash
# 使用 tito 构建
tito build --rpm --test

# 或手动构建
rpmbuild -ba ibus-libime.spec
```

## 发布新版本

发布新版本通常包括以下步骤：

```bash
# Bump version number in VERSION file
git add VERSION
git commit -m "Bump to version $(cat VERSION)"
VERSION=$(cat VERSION)
git tag -a v$VERSION -m "Release v$VERSION"
tito tag --use-version=$VERSION
tito build --offline \
  --tag="ibus-libime-$VERSION-1" \
  --tgz --rpm \
  --output="$PWD/tito-out/tito-$VERSION"

git push origin main --tags
```