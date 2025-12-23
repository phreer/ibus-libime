# IBus LibIME

一个基于 LibIME 的现代化 IBus 中文拼音输入法前端。

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

# 模糊音标志 (默认: 0，使用内置规则)
# 具体值参考 LibIME 的 PinyinFuzzyFlag 枚举
fuzzyflags=0
```

支持的配置项：
- **loglevel**: 日志级别，可选 DEBUG、INFO、WARN、ERROR
- **nbest**: 生成的候选词数量，影响选词准确度
- **pagesize**: 每页显示的候选词数量，建议设置为 9 或 10
- **fuzzyflags**: 模糊音标志，0 表示使用默认的 Inner + CommonTypo

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

