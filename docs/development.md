# 打包与发布

## Debian / Ubuntu 打包

支持 Ubuntu 24.04、Ubuntu 26.04 和 Debian 13，构建入口目前仅支持 amd64。
每个发行版需在对应系统内单独编译，不能将一个系统的 deb 当作其他系统的包。
Fedora 用户可使用对应发行版的容器或虚拟机；不要直接在 Fedora 上运行
`dpkg-buildpackage` 并将其产物作为 Debian 包发布。

### 本地构建

在源码根目录执行：

```bash
sudo apt-get update
sudo apt-get install --no-install-recommends dpkg-dev devscripts lintian
sudo apt-get build-dep --no-install-recommends .
./scripts/build-deb.sh
```

`apt-get build-dep .` 从 `debian/control` 安装完整构建和测试依赖，不需要
添加 deb-src。Ubuntu 软件源需启用 universe（官方 Ubuntu 容器默认已启用）。
构建使用系统 LibIME，不要求安装或运行 Fcitx 输入法。

脚本检查系统、架构、版本和依赖，在临时目录复制必要源码并执行
`dpkg-buildpackage --build=binary --no-sign`，包含现有全部 CTest 测试。
未提交的源码修改会参与构建；已有 build、RPM 等目录不会被打包或清理。
请勿设置 `DEB_BUILD_OPTIONS=nocheck` 或 `DEB_BUILD_PROFILES=nocheck`
来跳过发布包测试。可用 `DEB_BUILD_OPTIONS=parallel=2` 限制构建并发。

产物保存在 `deb-out/<VERSION>/<发行版>/`，例如：

```text
deb-out/0.1.2/ubuntu24.04/ibus-libime_0.1.2-1+ubuntu24.04_amd64.deb
deb-out/0.1.2/ubuntu26.04/ibus-libime_0.1.2-1+ubuntu26.04_amd64.deb
deb-out/0.1.2/debian13/ibus-libime_0.1.2-1+debian13_amd64.deb
```

同目录还包含调试符号包以及 `.changes`、`.buildinfo`。
`debian/changelog` 的上游版本必须与 `VERSION` 一致；`-1` 是 Debian
打包修订号，发行版后缀只添加到临时副本，不修改工作区 changelog。

### 检查与安装

以 Ubuntu 24.04 为例（按实际版本、修订号和发行版调整路径）：

```bash
lintian --fail-on error deb-out/0.1.2/ubuntu24.04/*.changes
./scripts/check-deb.sh deb-out/0.1.2/ubuntu24.04/ibus-libime_0.1.2-1+ubuntu24.04_amd64.deb
sudo apt install ./deb-out/0.1.2/ubuntu24.04/ibus-libime_0.1.2-1+ubuntu24.04_amd64.deb
ibus restart
```

`check-deb.sh` 应在对应目标系统运行，检查包名、版本、架构、数据依赖和
文件布局。APT 会安装 IBus、LibIME 字典、中文语言模型及共享库依赖。
无需手动复制字典，也不使用 `dpkg -i` 绕过依赖安装。
引擎和设置程序安装到 `/usr/libexec/`，IBus XML 与桌面入口引用相同路径。
安装包不会强制重启用户正在使用的 IBus 会话。

### CI

`.github/workflows/deb.yml` 在 push、pull request 和手动运行时，
分别在三个发行版的 amd64 容器中构建，运行测试、Lintian 和包结构检查，
并在干净容器中通过 APT 安装、核对动态链接和系统字典/模型。
主包和调试包等以 `deb-ubuntu24.04-amd64`、`deb-ubuntu26.04-amd64`、
`deb-debian13-amd64` 三个 Actions artifacts 保存。

CI 不创建 GitHub Release。为发布构建时，应选择对应发布标签运行工作流，
将三个 artifacts 下载到统一输出目录后再执行本地发布脚本：

```bash
# 将 RUN_ID 替换为已成功完成构建和安装检查的工作流运行 ID
gh run download RUN_ID -n deb-ubuntu24.04-amd64 -D deb-out
gh run download RUN_ID -n deb-ubuntu26.04-amd64 -D deb-out
gh run download RUN_ID -n deb-debian13-amd64 -D deb-out
```

artifact 内已保留 `<VERSION>/<发行版>/` 层级，不要再套一层 artifact 名目录。

## RPM 打包

```bash
# 使用 tito 构建
tito build --rpm --test

# 或手动构建
rpmbuild -ba ibus-libime.spec
```

## 发布新版本

发布新版本通常包括以下步骤：

```bash
# 修改 VERSION，同时更新 Debian changelog 的上游版本
# 将下行的 X.Y.Z 替换为新版本
dch --newversion X.Y.Z-1 --distribution unstable "New upstream release."
git add VERSION debian/changelog
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

`dch` 由 Debian/Ubuntu 的 devscripts 提供；在 Fedora 上也可以手动维护
`debian/changelog` 的标准格式。Tito 保持现有版本同步方式。
推送后等待对应提交/标签的 deb CI 完成，按上节下载三个发行版的产物。
确认 RPM 和 deb 都来自本次发布的相同源码后执行：

```bash
# 默认要求三个 RPM 和三个发行版的 deb 主包全部存在，上传可用的 deb 调试包
./scripts/create-gh-release.sh

# 仅发布一种格式时需显式选择；RPM-only 保留原来的上传范围
./scripts/create-gh-release.sh --rpm-only
./scripts/create-gh-release.sh --deb-only
```

脚本发布前会检查全部选定文件，缺包则失败，不会先创建不完整 Release。
`FC_VERSION` 默认为 `44`，可通过环境变量覆盖；`TITLE` 和 `MESSAGE`
仍可覆盖标题与说明。脚本保持现有 Release 标签名 `<VERSION>`，
不自动改成上例的 Git 标签 `v<VERSION>` 或 Tito 标签
`ibus-libime-<VERSION>-1`；发布者需注意这三个标签约定的区别。
默认模式相较原脚本新增了 deb 完整性检查，只发布 RPM 时使用 `--rpm-only`。