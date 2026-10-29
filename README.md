# IBus LibIME

一个基于 [LibIME](https://github.com/fcitx/libime) 的现代化 IBus 中文拼音输入法前端。

## 编译安装

### 从源码编译

```bash
# 创建构建目录
mkdir builddir
cd builddir

# 配置
cmake ..

# 编译
make -j$(nproc)

# 安装（需要 root 权限）
sudo make install
```

### RPM 打包（Fedora/RHEL）

```bash
# 使用 tito 构建
tito build --rpm

# 或手动构建
rpmbuild -ba ibus-libime.spec
```

## 使用说明

### 启用输入法

1. 安装后重启 IBus：
```bash
ibus restart
```

2. 在 IBus 设置中添加 "LibIME Pinyin" 输入法

3. 切换到该输入法开始使用

### 键盘快捷键

#### 输入控制
- **字母键 (a-z)**: 输入拼音
- **'（单引号）**: 拼音分隔符
- **空格**: 选择第一个候选词
- **数字键 1-9**: 选择对应候选词
- **数字键 0**: 选择第 10 个候选词

#### 编辑操作
- **BackSpace**: 删除一个拼音字符
- **Delete**: 删除光标处字符
- **Escape**: 取消当前输入
- **Enter**: 提交原始拼音（不转换）
- **左/右箭头**: 移动光标

#### 翻页导航
- **Page Up / -（减号）**: 上一页候选词
- **Page Down / =（等号）**: 下一页候选词

#### 模式切换
- **Shift**: 快速切换中英文模式
  - 按下 Shift 后松开：切换模式
  - 有输入时按 Shift：提交原始拼音并切换模式
- **托盘菜单中的 输入模式：中 / 输入模式：En**：切换最近的实际输入上下文的模式，打开菜单或返回输入框
  不会改变该模式。IBus 提供不同上下文 ID 时，各上下文独立记忆模式。
  GNOME Wayland 下多个应用可能共用 `gnome-shell` 上下文；此时模式和托盘状态共享，
  引擎无法仅根据 IBus 的焦点通知区分这些应用。


## 配置

### 引擎配置

在 `$HOME/.config/ibus-libime.ini` 中可以配置以下选项：

```ini
[general]
# 日志级别: DEBUG, INFO, WARN, ERROR
loglevel=INFO

# 候选词数量 (默认: 3)
nbest=3

# 每页显示的候选词数量 (默认: 9)
pagesize=9

# 中文模式下提供英文前缀候选 (默认: true)
englishcandidates=true

# 记录确认提交的中文输入历史 (默认: true)
recordinputhistory=true

# 学习已选择的中文候选并更新用户词库 (默认: true)
learnuserdictionary=true

# 模糊音标志 (默认: 空，使用内置规则)
# 可以使用逗号分隔的标志名称，支持的标志有：
# CommonTypo, V_U, AN_ANG, EN_ENG, IAN_IANG, IN_ING, U_OU, UAN_UANG,
# C_CH, F_H, L_N, S_SH, Z_ZH, VE_UE, Inner, InnerShort, PartialFinal,
# PartialSp, AdvancedTypo, Correction, L_R
fuzzyflags=Inner,CommonTypo
```

支持的配置项：
- **loglevel**: 日志级别，可选 DEBUG、INFO、WARN、ERROR
- **nbest**: 生成的候选词数量，影响选词准确度
- **pagesize**: 每页显示的候选词数量，建议设置为 9 或 10
- **englishcandidates**: 中文模式下的英文前缀补全，默认 `true`；设为 `false` 关闭
- **recordinputhistory**: 是否追加记录确认提交的中文输入历史，默认 `true`；
  设为 `false` 后不再写入 `input-history.tsv`
- **learnuserdictionary**: 是否通过已选中文候选学习并更新用户词库，默认 `true`；
  设为 `false` 后停止新增学习，但仍加载和使用已有用户数据
- **fuzzyflags**: 模糊音标志，使用逗号分隔的标志名称
  - **CommonTypo**: 常见错误（如 ng/gn）
  - **Inner**: 内部模糊音
  - **V_U**: v/u 不分
  - **AN_ANG**: an/ang 不分
  - **EN_ENG**: en/eng 不分
  - **IAN_IANG**: ian/iang 不分
  - **IN_ING**: in/ing 不分
  - **C_CH**: c/ch 不分
  - **S_SH**: s/sh 不分
  - **Z_ZH**: z/zh 不分
  - **L_N**: l/n 不分
  - 更多标志参考 LibIME 的 PinyinFuzzyFlag 枚举

### 输入历史

确认提交的中文候选会以追加式 TSV 保存到
`$XDG_DATA_HOME/ibus-libime/input-history.tsv`（通常为
`~/.local/share/ibus-libime/input-history.tsv`）。每行两列：拼音和提交的中文文本；
重复选择会保留为多行，便于统计词频并整理个性化词库。英文候选、Enter 提交的原始拼音
以及取消的输入不会记录。文件保存在本机用户数据目录中，不会上传。

### 中文模式英文补全

在中文模式输入至少 4 个连续小写英文字母时，会提供最多 2 个英文候选。
完整单个拼音音节（如 `zhi`、`shang`、`zhong`）不插入英文候选。
前缀补全仅召回词频至少 1000 的词条，避免 `zhiv` 补出低频的 `zhivago`。
完整单词匹配和技术词补充表不受词频门槛限制；长度和拼音音节限制仍适用。
例如输入 `comput` 可以选择 `computer`、`computers`；输入完整的 `hello` 时，
`hello` 优先于它的其他英文补全。通常保留中文首选，英文候选从第 2 位起插入。
拼音候选只展示连续可转换的前缀，不附加后面无法转换的原始字母，
不依赖是否命中英文。例如输入 `git` 显示 `g` 对应的单字候选，而不是“个it”；
选中后只消费 `g`，`it` 留在预编辑区继续编辑，不会丢弃。
剩余输入无法转换且没有其他候选时，候选框显示独立的原文候选 `it`；
可用空格、数字或鼠标选中，与已选汉字一起提交，不写入拼音学习词典。
输入完整匹配英文单词且原中文首选只能转换部分输入时，英文排第 1 位。
没有中文候选时，英文也从第 1 位开始。Enter 始终可以提交原始输入。
英文候选按完整匹配优先、词频其次排序；仅前缀匹配不会抢占中文首选。

检索使用小写字母，显示和提交保留词条的大小写。独立的
[技术词补充表](data/english-technical.txt) 收录 `Android`、`Linux`、`GitHub`，
输入 `andro` 即可选择 `Android`；小写 `android` 也仍可选择。
补充词继承基础词库中同名小写词的词频（没有则为零），同频时优先规范拼写。
不会自动将普通英文单词首字母大写。补充表每行一个纯英文字母词条，
更新后需重新安装并重启引擎。

数字键、空格和鼠标均可选词，英文选中后不附加空格，也不写入中文学习词典。
Enter 仍提交原始输入。En 模式仍然透传全部按键，不显示补全。
光标不在输入末尾、已经部分选词或输入含拼音分隔符时，不提供英文补全。
可设置 `englishcandidates=false` 关闭本功能，修改后重启引擎生效。

英文词库随安装包提供，无需联网。数据来自 Hermit Dave 及贡献者的
[FrequencyWords](https://github.com/hermitdave/FrequencyWords)，基于 OpenSubtitles2018，
以 CC BY-SA 4.0 发布；详见 [来源说明](data/english-README.txt) 和
[数据许可证](data/english-LICENSE.txt)。程序许可证不变。
词库加载失败会在日志中记录警告，中文输入不受影响。
开发时可用 `IBUS_LIBIME_DATA_DIR` 指定同时包含 `english-frequency.txt`
和 `english-technical.txt` 的目录；
该变量与指定中文词典位置的 `LIBIME_DATA_DIR` 独立。

## 故障排查

### 输入法未显示

```bash
# 检查是否安装
ls /usr/libexec/ibus-engine-libime

# 检查组件文件
ls /usr/share/ibus/component/libime.xml

# 重启 IBus
ibus restart

# 查看日志
journalctl -f | grep ibus
```

### 词典加载失败

确保安装了 `libime-data` 包，并且词典文件存在：
```bash
ls /usr/share/libime/sc.dict
```

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

GPL-3.0

## 相关链接

- [LibIME 项目](https://github.com/fcitx/libime)
- [IBus 文档](https://github.com/ibus/ibus)
