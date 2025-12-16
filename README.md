# IBus LibIME Frontend

这是一个基于 LibIME 的 IBus 输入法前端实现。

## 功能特性

- 拼音输入支持
- 智能候选词排序
- 用户词典学习
- 模糊音支持
- 翻页和候选词选择

## 编译

```bash
cd ibus
mkdir build
cd build
cmake ..
make
sudo make install
```

## 依赖

- IBus >= 1.5.0
- GLib >= 2.0
- LibIME (Core 和 Pinyin 模块)

## 使用

1. 编译安装后，重启 IBus：
```bash
ibus restart
```

2. 在 IBus 设置中添加 "LibIME Pinyin" 输入法

3. 切换到该输入法开始使用

## 键盘快捷键

- **字母键**: 输入拼音
- **数字键 1-9, 0**: 选择候选词
- **空格**: 选择第一个候选词
- **BackSpace**: 删除
- **Escape**: 取消输入
- **左/右箭头**: 移动光标
- **Page Up/Down 或 -/=**: 翻页
- **Enter**: 提交当前句子

## 配置

数据文件路径可以通过环境变量 `LIBIME_DATA_DIR` 指定，默认为 `/usr/share/libime/`

## 项目结构

```
ibus/
├── CMakeLists.txt          # CMake 配置
├── README.md              # 本文件
├── data/
│   └── libime.xml.in      # IBus 组件配置
└── src/
    ├── main.cpp           # 入口
    ├── ibus_engine.h      # IBus 引擎封装
    ├── ibus_engine.cpp
    ├── pinyin_engine.h    # 拼音引擎实现
    └── pinyin_engine.cpp
```

## API 使用

本项目使用 LibIME 提供的以下核心 API：

- `PinyinIME`: 全局配置和资源管理
- `PinyinContext`: 输入上下文和状态管理
- `PinyinDictionary`: 词典加载和管理
- `UserLanguageModel`: 用户语言模型

详细 API 文档请参考 `../doc/API.md`

## 许可证

LGPL-2.1-or-later
