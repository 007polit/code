# 🔧 Editer 安装配置指南

完整的 Windows 平台安装、配置和故障排除指南。

## 🚀 快速开始 (5分钟上手)

### 第1步：获取项目
```bash
# 方法A: Git克隆 (推荐)
git clone <your-repository-url>
cd editer

# 方法B: 下载ZIP
# 1. 下载项目ZIP文件
# 2. 解压到任意目录
# 3. 打开终端，进入项目目录
```

### 第2步：安装依赖库
```bash
# 如果你已经有vcpkg：
vcpkg install pdcurses:x64-windows curl:x64-windows nlohmann-json:x64-windows openssl:x64-windows

# 如果你没有vcpkg，请看下面的详细安装说明
```

### 第3步：编译项目
```bash
# 运行构建脚本
build_msvc.bat

# 如果成功，会生成 editer.exe
```

### 第4步：开始使用
```bash
# 编辑新文件
editer.exe

# 编辑现有文件
editer.exe examples\test.cpp

# 基本操作：i(插入) ESC(普通) Ctrl+S(保存) Ctrl+Q(退出)
```

---

## 🎯 系统要求

- **操作系统**: Windows 10/11 (64位)
- **编译器**: MSVC 2019 或更高版本 (支持 C++20)
- **内存**: 最少 4GB RAM (推荐 8GB+，用于 AI 功能)
- **磁盘**: 500MB 可用空间 (AI 模型需要更多)
- **Visual Studio**: 建议安装 Visual Studio 2019/2022 或 Build Tools

## 📦 详细依赖库安装

### 1. 安装 Visual Studio 或 Build Tools

#### 选项A: Visual Studio (推荐)
1. 下载 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/zh-hans/downloads/)
2. 安装时选择 "使用 C++ 的桌面开发" 工作负载
3. 确保包含 MSVC v142 或更高版本编译器

#### 选项B: Build Tools (轻量级)
1. 下载 [Build Tools for Visual Studio](https://visualstudio.microsoft.com/zh-hans/downloads/#build-tools-for-visual-studio-2022)
2. 安装 "C++ 生成工具" 工作负载
3. 包含 Windows 10 SDK

### 2. 安装 vcpkg (包管理器)

```bash
# 克隆 vcpkg 仓库
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# 运行 bootstrap 脚本
.\bootstrap-vcpkg.bat

# 集成到系统 (需要管理员权限)
.\vcpkg integrate install
```

### 3. 安装必需库

```bash
# 安装所有依赖库 (需要一些时间)
vcpkg install pdcurses:x64-windows
vcpkg install curl:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install openssl:x64-windows

# 验证安装
vcpkg list | findstr "pdcurses curl nlohmann openssl"
```

**预期输出**:
```
pdcurses:x64-windows
curl:x64-windows
nlohmann-json:x64-windows
openssl:x64-windows
```

### 4. 设置环境变量

#### 方法A: 通过系统设置
1. 右键 "此电脑" → "属性"
2. "高级系统设置" → "环境变量"
3. 在 "系统变量" 中找到 `Path`
4. 添加 vcpkg 目录路径，例如: `C:\vcpkg`

#### 方法B: 通过 PowerShell (临时)
```powershell
$env:Path += ";C:\vcpkg"
```

## 🔨 编译项目

### 使用构建脚本 (推荐)

```bash
# 进入项目目录
cd path\to\editer

# 运行 MSVC 构建脚本
build_msvc.bat
```

**构建脚本会自动**:
1. 设置 MSVC 环境变量
2. 检查编译器和 vcpkg
3. 编译所有源文件
4. 链接依赖库
5. 生成 `editer.exe`

### 手动编译 (高级)

如果需要自定义编译选项：

```bash
# 设置 MSVC 环境 (根据你的 VS 安装路径调整)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 编译
cl /std:c++20 /EHsc /I"include" /I"C:\vcpkg\installed\x64-windows\include" ^
   src\main.cpp src\file_tree.cpp src\syntax_renderer.cpp ^
   src\ui\terminal.cpp src\ai\*.cpp src\config\*.cpp src\syntax\*.cpp ^
   /link /LIBPATH:"C:\vcpkg\installed\x64-windows\lib" ^
   pdcurses.lib libcurl.lib libssl.lib libcrypto.lib ^
   /OUT:editer.exe
```

## 🚀 运行程序

### 基本使用

```bash
# 创建新文件
editer.exe

# 编辑现有文件
editer.exe myfile.cpp

# 编辑示例文件
editer.exe examples\test.cpp
```

### 首次运行

1. **启动程序**
   ```bash
   editer.exe examples\test.cpp
   ```

2. **界面说明**
   - 中间区域：文本编辑器
   - 左侧 (Ctrl+F)：文件树
   - 右侧 (Ctrl+A)：AI 聊天
   - 底部 (Ctrl+T)：终端 (5行)
   - 最底部：状态栏

3. **基本操作**
   - 按 `i` 进入插入模式
   - 输入代码
   - 按 `ESC` 返回普通模式
   - 按 `Ctrl+S` 保存文件
   - 按 `Ctrl+Q` 退出程序

4. **第一次编辑体验**
   ```cpp
   #include <iostream>
   
   int main() {
       std::cout << "Hello Editer!" << std::endl;
       return 0;
   }
   ```

## 🤖 AI 功能配置

Editer 通过远端 OpenAI 风格 HTTP API 提供 AI 对话与代码辅助功能（例如 `https://llmapi.paratera.com/v1`）。

### 1. 配置 API

在项目根目录的 `config.cfg` 中设置：

```cfg
# API endpoint URL (base URL without /v1)
set api_url = https://llmapi.paratera.com

# API key (required)
set api_key = your_api_key_here

# Model name/ID to use
set api_model = DeepSeek-R1
```

### 2. 使用 AI 功能

1. 启动 Editer
2. 按 `Ctrl+A` 打开 AI 聊天窗口
3. 输入问题或代码请求
4. 按 `Enter` 发送消息
5. 等待远端模型返回结果

**示例对话**:
```
你: 解释这段 C++ 代码的作用
AI: 这段代码实现了一个简单的文本缓冲区管理...

你: 帮我重构这个函数，让可读性更好
AI: 可以将逻辑拆分为三个小函数，分别负责...
```

### AI 模型推荐

| 模型 | 适用场景 |
|------|----------|
| `DeepSeek-R1` | 通用对话与代码解释 |
| `Qwen3-Coder-480B-A35B-Instruct` | 偏重代码相关对话 |

## ⚙️ 配置文件

### config.cfg

编辑项目根目录的 `config.cfg`:

```cfg
# Tab 大小 (每个 Tab 的空格数)
set tabsize = 4
```

**支持的设置**:

| 设置项 | 类型 | 默认值 | 范围 | 说明 |
|--------|------|--------|------|------|
| `tabsize` | int | 4 | 1-16 | Tab 键插入的空格数 |

编辑器会自动将 `config.cfg` 转换为 `config.json` 以加快加载速度。

### editer.log

程序运行时会生成 `editer.log` 日志文件，包含：
- 应用程序启动信息
- 组件初始化状态
- 错误和警告信息
- 布局计算详情

**日志级别**:
- `DEBUG` - 调试信息
- `INFO` - 一般信息
- `WARNING` - 警告信息
- `ERROR` - 错误信息

## 🔧 故障排除

### 编译错误

#### "MSVC 未找到"
```bash
# 确保已安装 Visual Studio 或 Build Tools
# 检查环境变量是否正确设置

# 手动设置 MSVC 环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

#### "vcpkg 未找到"
```bash
# 检查 vcpkg 是否在 PATH 中
where vcpkg

# 如果没有，添加到 PATH 或使用完整路径
C:\vcpkg\vcpkg install pdcurses:x64-windows
```

#### "库文件未找到"
```bash
# 检查 vcpkg 安装
vcpkg list

# 重新安装缺失的库
vcpkg install <library-name>:x64-windows

# 确保使用 x64-windows 平台
vcpkg install pdcurses:x64-windows --triplet x64-windows
```

#### "C++20 不支持"
```bash
# 检查 MSVC 版本
cl

# 确保使用 MSVC 2019 或更高版本
# 更新 Visual Studio 到最新版本
```

#### "链接错误"
```bash
# 检查库文件路径
dir C:\vcpkg\installed\x64-windows\lib

# 确保 build_msvc.bat 中的路径正确
# 根据你的 vcpkg 安装位置调整路径
```

### 运行时错误

#### "无法初始化渲染器"
- 确保终端支持颜色显示
- 检查 PDCurses 是否正确安装
- 尝试在不同的终端中运行 (PowerShell, CMD, Windows Terminal)
- 检查 `editer.log` 查看详细错误信息

#### "配置文件错误"
- 删除 `config.cfg` 和 `config.json` 让程序重新创建
- 检查文件权限
- 确保配置文件格式正确

#### "AI 聊天不可用"
- 检查 `config.cfg` 中的 `api_url`、`api_key`、`api_model` 是否填写正确
- 使用 `curl` 在命令行中测试 API 是否可用
- 查看 `editer.log` 中的 HTTP 错误码和响应内容

#### "终端不显示"
- 终端可见时始终占据底部 5 行
- 按 `Ctrl+T` 切换显示/隐藏
- 检查 `editer.log` 中的布局信息
- 确保屏幕高度足够 (至少 20 行)

#### "文件树无法打开文件"
- 确保文件有读取权限
- 检查文件路径是否正确
- 查看 `editer.log` 中的错误信息

### 常见问题解答

**Q: 编译失败怎么办？**  
A: 检查依赖库是否正确安装，确保编译器支持 C++20，查看 `editer.log` 获取详细错误信息。

**Q: 如何退出程序？**  
A: 按 `Ctrl+Q` 或者在命令模式下输入 `:q`。

**Q: 如何保存文件？**  
A: 按 `Ctrl+S` 或者在命令模式下输入 `:w`。

**Q: 支持哪些文件类型？**  
A: 支持 C++、Python、Markdown、JSON、CFG 的语法高亮，可以编辑任何文本文件。

**Q: AI 功能需要联网吗？**  
A: 需要。当前版本通过远端 HTTP API 调用大模型服务，请确保网络连接和 API 配置正确。

**Q: 为什么 AI 响应很慢？**  
A: 取决于你的硬件配置和模型大小。建议使用 7B 模型配合 8GB+ 内存，或使用 1.5B 轻量级模型。

**Q: 可以同时编辑多个文件吗？**  
A: 当前版本只支持单文件编辑，多文件标签功能在未来版本中计划实现。

**Q: 如何更改主题颜色？**  
A: 当前版本使用默认主题，自定义主题功能在未来版本中计划实现。

## 🆘 获取帮助

如果遇到问题：

1. **检查日志**: 查看 `editer.log` 文件
2. **检查依赖**: 确保所有库都正确安装
3. **重新编译**: 清理后重新构建
4. **检查版本**: 确保编译器支持 C++20

### 调试技巧

```bash
# 详细编译信息
cl /std:c++20 /EHsc /I"include" ... /verbose

# 检查库依赖
dumpbin /dependents editer.exe

# 查看日志
type editer.log

# 清理重新构建
del *.obj *.exe
build_msvc.bat
```

### 性能优化

如果程序运行缓慢：

1. **关闭不需要的组件**
   - 不使用文件树时按 `Ctrl+F` 关闭
   - 不使用 AI 时按 `Ctrl+A` 关闭
   - 不使用终端时按 `Ctrl+T` 关闭

2. **使用较小的远端模型或降低 max_tokens**

3. **减少日志输出**
   - 编辑 `include/utils/logger.hpp`
   - 将日志级别改为 `INFO` 或 `WARNING`

## 🎉 安装完成

现在你已经成功安装了 Editer，可以开始享受高效的编程体验了！

### 接下来你可以：
- 📖 阅读 [README.md](README.md) 了解详细使用方法
- 🏗️ 阅读 [ARCHITECTURE.md](ARCHITECTURE.md) 了解系统架构
- 🎮 探索各种快捷键和功能
- 🤖 配置 AI 助手提升编码效率
- 🔧 自定义配置文件

### 快速参考

**全局快捷键**:
- `Ctrl+S` - 保存
- `Ctrl+Q` - 退出
- `Ctrl+F` - 文件树
- `Ctrl+T` - 终端
- `Ctrl+A` - AI 聊天

**编辑模式**:
- `i` - 插入模式
- `ESC` - 普通模式
- `v` - 可视模式
- `:` - 命令模式

**命令**:
- `:w` - 保存
- `:q` - 退出
- `:wq` - 保存并退出
- `:e <file>` - 打开文件

---

**Happy Coding with Editer! 🚀✨**
