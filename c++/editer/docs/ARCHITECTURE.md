# Editer 架构文档

本文档描述 Editer 文本编辑器的架构和设计。

## 系统概述

Editer 是一个基于终端的文本编辑器，使用 C++20 构建，采用模块化架构，各模块职责清晰分离。系统遵循基于组件的设计，每个模块处理特定的职责。

## 架构图

### 整体系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         EditerApp                               │
│                      (主应用控制器)                              │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │FocusManager  │  │LayoutManager │  │    Logger    │         │
│  │  焦点管理    │  │  布局管理    │  │   日志系统    │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │  Editor  │  │ Renderer │  │ FileTree │  │ Terminal │      │
│  │  编辑器   │  │  渲染器   │  │  文件树   │  │   终端    │      │
│  └────┬─────┘  └────┬─────┘  └──────────┘  └──────────┘      │
│       │             │                                           │
│  ┌────▼─────┐  ┌────▼──────────────┐  ┌──────────┐           │
│  │  Buffer  │  │ SyntaxRenderer +  │  │  AIChat  │           │
│  │  缓冲区   │  │  Highlighters     │  │ AI聊天   │           │
│  │          │  │                   │  │          │           │
│  └──────────┘  │ - C++Highlighter  │  └────┬─────┘           │
│                │ - PythonHighlight │       │                  │
│                │ - JSONHighlighter │  ┌────▼──────────────┐  │
│                │ - MarkdownHigh.   │  │  APIClient       │  │
│                │ - CfgHighlighter  │  │ (HTTP通信)       │  │
│                └───────────────────┘  └──────────────────┘  │
│                                                                 │
│  ┌──────────────────────────┐  ┌──────────────────────────┐  │
│  │   ConfigInterpreter      │  │    Config Module         │  │
│  │   (配置文件解析)           │  │   (配置管理)             │  │
│  └──────────────────────────┘  └──────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │  NCurses/PDC    │
                 │   (UI库)        │
                 └─────────────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │    终端屏幕      │
                 └─────────────────┘
```

### 详细类关系图

```
┌────────────────────────────────────────────────────────────────┐
│                     核心编辑模块                                │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐         ┌──────────────┐                  │
│  │   Editor     │────────▶│   Buffer     │                  │
│  ├──────────────┤         ├──────────────┤                  │
│  │ - cursor_    │         │ - lines_     │                  │
│  │ - mode_      │         │ - undo_stack │                  │
│  │ - buffer_    │         │ - redo_stack │                  │
│  │ - undo_stack │         │              │                  │
│  │ - redo_stack │         └──────────────┘                  │
│  │              │                                            │
│  │ + move_()    │         ┌──────────────┐                  │
│  │ + insert()   │────────▶│   Action     │                  │
│  │ + delete()   │         ├──────────────┤                  │
│  │ + undo()     │         │ - type       │                  │
│  │ + redo()     │         │ - pos        │                  │
│  │ + find()     │         │ - text       │                  │
│  │              │         └──────────────┘                  │
│  └──────────────┘                                            │
│                                                                │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                      UI 渲染模块                                │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐                                            │
│  │  Renderer    │                                            │
│  ├──────────────┤                                            │
│  │ - window_    │       ┌──────────────────────┐            │
│  │ - width_     │      │  SyntaxRenderer      │            │
│  │ - height_    │      ├──────────────────────┤            │
│  │ - buffer_    │      │ - highlighter_       │            │
│  │              │      │ - syntax_ranges_     │            │
│  │ + render()   │─────▶│                      │            │
│  │ + show_msg() │      │ + render_line()      │            │
│  │ + draw_line()│      │ + get_colors()       │            │
│  │              │      └──┬───────────────────┘            │
│  └──────────────┘         │                                │
│                           │                                 │
│                    ┌──────▼──────────────┐                 │
│                    │   Highlighter(基类) │                 │
│                    ├───────────────────┤                  │
│                    │ + tokenize()       │                 │
│                    │ + get_color()      │                 │
│                    └──────┬─────────────┘                 │
│                           │                               │
│          ┌────────────────┼────────────────┬─────────┐   │
│          │                │                │         │   │
│    ┌─────▼────┐  ┌────────▼───┐  ┌──────▼──┐  ┌──────▼──┐ │
│    │C++High.  │  │PythonHigh. │  │JSONHigh.│  │MarkHigh│ │
│    └──────────┘  └────────────┘  └─────────┘  └────────┘ │
│                                                                │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                     文件管理模块                                │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐                                            │
│  │  FileTree    │         ┌──────────────┐                  │
│  ├──────────────┤         │  TreeNode    │                  │
│  │ - root_      │────────▶├──────────────┤                  │
│  │ - selected_  │         │ - name       │                  │
│  │ - visible_   │         │ - full_path  │                  │
│  │ - window_    │         │ - is_dir     │                  │
│  │              │         │ - is_expnd   │                  │
│  │ + expand()   │         │ - children   │                  │
│  │ + collapse() │         │ - parent     │                  │
│  │ + create()   │         └──────────────┘                  │
│  │ + delete()   │                                            │
│  │ + rename()   │         ┌──────────────┐                  │
│  │ + cut/copy/  │────────▶│FileOperation │                  │
│  │   paste()    │         ├──────────────┤                  │
│  │              │         │None, Create, │                  │
│  └──────────────┘         │ Delete, etc. │                  │
│                           └──────────────┘                  │
│                                                                │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                      终端模块                                  │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐                                            │
│  │  Terminal    │                                            │
│  ├──────────────┤                                            │
│  │ - window_    │         ┌──────────────┐                  │
│  │ - input_buf_ │────────▶│ 命令执行引擎 │                  │
│  │ - history_   │         ├──────────────┤                  │
│  │ - output_buf │         │ + execute()  │                  │
│  │              │         │ + cd/mkdir   │                  │
│  │ + handle_in()│         │ + ls/pwd     │                  │
│  │ + execute()  │         └──────────────┘                  │
│  │ + render()   │                                            │
│  │ + complete() │                                            │
│  │              │                                            │
│  └──────────────┘                                            │
│                                                                │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                      AI 集成模块                                │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐         ┌──────────────┐                  │
│  │  AIChat      │────────▶│ APIClient    │                  │
│  ├──────────────┤         ├──────────────┤                  │
│  │ - history_   │         │ - api_url_   │                  │
│  │ - messages_  │         │ - api_key_   │                  │
│  │ - client_    │         │ - model_name_│                  │
│  │              │         │ - available_ │                  │
│  │ + chat()     │         │ + chat()     │                  │
│  │ + clear()    │         │ + list_models│                  │
│  │ + save_hist()│         └──────────────┘                  │
│  │              │                │                           │
│  └──────────────┘                │                           │
│                                  ▼                            │
│                        ┌─────────────────────────┐          │
│                        │ OpenAI 风格 HTTP API    │          │
│                        │ (如 llmapi.paratera.com)│          │
│                        └─────────────────────────┘          │
│                                                                │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                    配置管理模块                                │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────────┐     ┌──────────────┐                  │
│  │ConfigInterpreter │────▶│    Config    │                  │
│  ├──────────────────┤     ├──────────────┤                  │
│  │ + parse()        │     │ - settings   │                  │
│  │ + interpret()    │     │ - keybinds   │                  │
│  │ + validate()     │     │              │                  │
│  └──────────────────┘     │ + get()      │                  │
│                           │ + set()      │                  │
│                           └──────────────┘                  │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. EditerApp (主控制器)

**位置**: `src/main.cpp`

**职责**:
- 应用程序生命周期管理 (初始化、运行、清理)
- 输入路由到相应组件
- 组件协调和通信
- 全局快捷键处理 (Ctrl+S, Ctrl+Q 等)

**关键方法**:
- `init()` - 初始化所有组件
- `run()` - 主事件循环
- `handle_input()` - 将输入路由到焦点组件
- `update_layout()` - 协调布局更新
- `cleanup()` - 清理关闭

### 2. 核心编辑组件

#### Editor (编辑器)

**位置**: `include/core/editor.hpp`

**职责**:
- 文本编辑操作 (插入、删除、移动)
- 模式管理 (Normal, Insert, Visual, Command)
- 光标位置跟踪
- 撤销/重做功能
- 文件 I/O 操作

**关键特性**:
- Vim 风格的模式编辑
- 命令执行 (`:w`, `:q`, `:e`)
- 基于单词的导航
- 行操作 (删除、复制、粘贴)

#### Buffer (缓冲区)

**位置**: `include/core/buffer.hpp`

**职责**:
- 文本存储和操作
- 基于行的文本管理
- 高效的文本操作
- 撤销/重做状态管理

**数据结构**:
```cpp
std::vector<std::string> lines_;  // 文本内容
std::stack<BufferState> undo_stack_;  // 撤销栈
std::stack<BufferState> redo_stack_;  // 重做栈
```

### 3. UI 组件

#### Renderer (渲染器)

**位置**: `include/ui/renderer.hpp`

**职责**:
- 屏幕渲染和更新
- 语法高亮显示
- 状态栏渲染
- 消息显示
- 光标定位

**关键特性**:
- 双缓冲实现平滑渲染
- 语法高亮颜色支持
- 行号显示
- 模式指示器

#### FileTree (文件树)

**位置**: `include/ui/file_tree.hpp`

**职责**:
- 文件系统导航
- 目录树显示
- 文件操作 (创建、删除、重命名)
- 文件移动 (剪切、复制、粘贴)

**数据结构**:
```cpp
struct TreeNode {
    std::string name;           // 文件/文件夹名
    std::string full_path;      // 完整路径
    bool is_directory;          // 是否为目录
    bool is_expanded;           // 是否展开
    std::vector<std::unique_ptr<TreeNode>> children;  // 子节点
};
```

**特性**:
- 递归目录加载
- 延迟加载以提高性能
- 文件类型颜色编码
- 剪贴板操作

#### Terminal (终端)

**位置**: `include/ui/terminal.hpp`

**职责**:
- 命令执行
- 输出显示
- 命令历史
- 可滚动输出缓冲区

**关键特性**:
- 全屏宽度 (始终)
- 固定 5 行高度
- 1000 行输出历史
- 50 条命令历史
- 滚动指示器

### 4. AI 集成

#### AIChat (AI 聊天)

**位置**: `include/ai/ai_chat.hpp`

**职责**:
- AI 对话管理
- 消息历史
- 与 APIClient 集成

#### APIClient (通用 AI API 客户端)

**位置**: `include/ai/api_client.hpp`

**职责**:
- 通过 HTTPS 与远端 AI 服务通信（OpenAI 兼容接口）
- 模型可用性检查（基于配置）
- 请求/响应处理
- 错误处理与日志记录

**通信流程**:
```
用户输入 → AIChat → APIClient → 远端 AI HTTP API
                                          ↓
用户显示 ← AIChat ← APIClient ← 响应
```

### 5. 语法高亮

#### Highlighter (基类)

**位置**: `include/syntax/highlighter.hpp`

**职责**:
- Token 类型定义
- 基础高亮接口
- 语言检测

#### 特定语言高亮器

**位置**:
- `include/syntax/cpp_highlighter.hpp` - C/C++
- `include/syntax/python_highlighter.hpp` - Python
- `include/syntax/markdown_highlighter.hpp` - Markdown
- `include/syntax/json_highlighter.hpp` - JSON
- `include/syntax/cfg_highlighter.hpp` - 配置文件

**Token 类型**:
- 关键字 (if, for, class 等)
- 字符串和字符
- 注释 (单行、多行)
- 数字
- 运算符
- 标识符
- 预处理指令

#### SyntaxRenderer (语法渲染器)

**位置**: `include/ui/syntax_renderer.hpp`

**职责**:
- 对文本应用语法高亮
- 颜色映射
- Token 渲染

### 6. 配置系统

#### ConfigInterpreter (配置解释器)

**位置**: `include/config/config_interpreter.hpp`

**职责**:
- 解析 DSL 配置文件 (`config.cfg`)
- 加载/保存 JSON 配置
- 提供配置值
- 默认值管理

**配置格式**:
```cfg
# DSL 格式
set tabsize = 4

# 转换为 JSON
{
  "tabsize": 4
}
```

### 7. 工具组件

#### Logger (日志系统)

**位置**: `include/utils/logger.hpp`

**职责**:
- 应用程序范围的日志记录
- 日志级别管理 (DEBUG, INFO, WARNING, ERROR)
- 文件输出 (`editer.log`)
- 线程安全日志

**使用方法**:
```cpp
LOG_INFO("应用程序已启动");
LOG_ERROR("加载文件失败: " + filename);
LOG_DEBUG("光标位置: " + pos);
```

#### Result 类型

**位置**: `include/utils/result.hpp`

**职责**:
- 错误处理
- 成功/失败指示
- 错误消息传播

**使用方法**:
```cpp
Result<std::string> load_file(const std::string& path) {
    if (!exists(path)) {
        return Result<std::string>::error("文件未找到");
    }
    return Result<std::string>::ok(content);
}
```

#### FocusManager (焦点管理器)

**位置**: `include/ui/focus_manager.hpp`

**职责**:
- 焦点栈管理
- 组件焦点跟踪
- 焦点转换

**焦点目标**:
- Editor (编辑器，默认)
- FileTree (文件树)
- Terminal (终端)
- AIChat (AI 聊天)

#### LayoutManager (布局管理器)

**位置**: `include/ui/layout_manager.hpp`

**职责**:
- 屏幕空间分配
- 组件定位
- 布局更新

**布局规则**:
- 终端: 全宽，底部 5 行
- 文件树: 左侧，自适应宽度
- 编辑器: 中间，剩余空间
- AI 聊天: 右侧，固定 50 列

## 类关系和依赖

### 继承关系

```
Highlighter (抽象基类)
    ↑
    ├── C++Highlighter
    ├── PythonHighlighter
    ├── JSONHighlighter
    ├── MarkdownHighlighter
    └── CfgHighlighter
```

### 聚合关系

```
EditerApp
    ├─ Editor
    │   └─ Buffer
    ├─ Renderer
    │   └─ SyntaxRenderer
    │       └─ Highlighter
    ├─ FileTree
    │   └─ TreeNode
    ├─ Terminal
    ├─ AIChat
    │   └─ APIClient
    ├─ FocusManager
    ├─ LayoutManager
    ├─ Logger
    └─ Config

```

### 关键交互

| 交互方向 | 源 | 目标 | 操作 | 说明 |
|--------|-----|-----|------|------|
| 编辑操作 | Editor | Buffer | insert/delete/replace | 文本操作 |
| 撤销/重做 | Editor | Buffer | undo/redo | 状态恢复 |
| 渲染 | Renderer | Editor | get_content/get_cursor | 获取显示数据 |
| 语法高亮 | SyntaxRenderer | Highlighter | tokenize/get_color | 获取高亮信息 |
| 文件操作 | FileTree | 文件系统 | create/delete/rename | 文件管理 |
| 命令执行 | Terminal | 系统 | execute_command | 执行外部命令 |
| AI对话 | AIChat | APIClient | send_request | 发送请求 |
| 焦点转换 | FocusManager | 各组件 | handle_input | 路由输入 |

## 数据流

### 1. 输入处理

```
用户输入
    ↓
EditerApp::handle_input()
    ↓
FocusManager::current()
    ↓
[Editor | FileTree | Terminal | AIChat]::handle_input()
    ↓
组件特定处理
    ↓
Renderer::render()
```

### 2. 文件操作

```
用户命令 (:e filename)
    ↓
Editor::execute_command()
    ↓
Buffer::load_from_file()
    ↓
LanguageDetector::detect()
    ↓
Highlighter::tokenize()
    ↓
Renderer::render_with_highlighting()
```

### 3. 布局更新

```
组件切换 (Ctrl+F/T/A)
    ↓
EditerApp::toggle_component()
    ↓
FocusManager::push/pop()
    ↓
LayoutManager::get_screen_config()
    ↓
LayoutManager::update()
    ↓
[FileTree | Renderer | Terminal | AIChat]::resize()
```

### 4. AI 交互

```
用户消息
    ↓
AIChat::handle_input()
    ↓
APIClient::chat()
    ↓
HTTP 请求 → 远端 AI HTTP API
    ↓
HTTP 响应 ← 远端 AI HTTP API
    ↓
AIChat::add_message()
    ↓
AIChat::render()
```

## 设计模式

### 1. 单例模式

**使用场景**:
- `Logger` - 单一全局日志实例
- 组件管理器

**优点**:
- 全局访问点
- 控制实例化
- 资源共享

### 2. 策略模式

**使用场景**:
- 语法高亮器 (不同语言的不同策略)

**优点**:
- 易于添加新语言
- 可插拔的高亮算法
- 运行时选择

### 3. 观察者模式 (隐式)

**使用场景**:
- 组件更新触发渲染器更新
- 焦点变化触发布局更新

### 4. 命令模式 (隐式)

**使用场景**:
- 命令模式操作 (`:w`, `:q`, `:e`)
- 撤销/重做功能

## 内存管理

### 智能指针

```cpp
// 所有权
std::unique_ptr<Editor> editor_;
std::unique_ptr<Renderer> renderer_;
std::unique_ptr<FocusManager> focus_manager_;

// 共享所有权 (需要时)
std::shared_ptr<Highlighter> highlighter_;
```

### 资源清理

- ncurses 窗口使用 RAII
- 析构函数中自动清理
- 复杂资源的显式清理方法

## 线程模型

**当前**: 单线程

**线程安全组件**:
- Logger (互斥锁保护)

**未来考虑**:
- 异步 AI 请求
- 后台文件操作
- 后台语法高亮

## 错误处理

### 策略

1. **Result 类型** 用于可能失败的操作
2. **日志记录** 用于调试和诊断
3. **优雅降级** 用于非关键功能 (如 AI)

### 示例

```cpp
Result<void> save_file() {
    if (!has_write_permission()) {
        LOG_ERROR("没有写入权限");
        return Result<void>::error("权限被拒绝");
    }
    
    if (!write_to_disk()) {
        LOG_ERROR("写入失败");
        return Result<void>::error("写入失败");
    }
    
    LOG_INFO("文件保存成功");
    return Result<void>::ok();
}
```

## 性能考虑

### 1. 渲染优化

- 只重绘变化的区域
- 双缓冲防止闪烁
- 延迟语法高亮

### 2. 文件树

- 延迟加载目录内容
- 只展开可见节点
- 缓存目录结构

### 3. 语法高亮

- 只对可见行进行标记化
- 缓存标记化结果
- 增量更新

### 4. 终端输出

- 循环缓冲区用于历史记录
- 限制历史大小 (1000 行)
- 高效滚动

## 配置

### 构建时配置

- `build_msvc.bat` 中的编译器标志
- 包含路径
- 库依赖

### 运行时配置

- `config.cfg` 用于用户设置
- `config.json` 用于快速加载
- 代码中的默认值

## 依赖

### 外部库

| 库 | 用途 | 版本 | 平台 |
|---|------|------|------|
| PDCurses | 终端 UI | 最新 | Windows |
| libcurl | HTTP 请求 (AI API) | 最新 | Windows |
| OpenSSL | HTTPS 支持 | 最新 | Windows |

### 依赖管理

- **vcpkg** 用于包管理
- 静态链接以提高可移植性
- 仅支持 Windows 平台

## 测试策略

### 当前状态

- 手动测试
- `examples/` 中的示例文件

### 未来计划

- 核心组件的单元测试
- 工作流的集成测试
- 自动化回归测试

## 扩展性

### 添加新功能

1. **新语法高亮器**:
   - 继承 `Highlighter`
   - 实现 `tokenize()`
   - 在 `LanguageDetector` 中注册

2. **新命令**:
   - 添加到 `Editor::execute_command()`
   - 在 README 中记录

3. **新 UI 组件**:
   - 创建组件类
   - 添加到 `EditerApp`
   - 更新 `LayoutManager`
   - 添加焦点目标到 `FocusManager`

## 构建系统

### MSVC 构建流程

```
build_msvc.bat
    ↓
设置 MSVC 环境 (vcvars64.bat)
    ↓
检查编译器和 vcpkg
    ↓
编译所有 .cpp 文件
    ↓
链接库
    ↓
生成 editer.exe
```

### 编译单元

- `src/main.cpp` - 主应用程序
- `src/file_tree.cpp` - 文件树实现
- `src/ui/terminal.cpp` - 终端实现
- `src/ai/*.cpp` - AI 集成
- `src/config/*.cpp` - 配置系统
- `src/syntax/*.cpp` - 语法高亮器
- `src/syntax_renderer.cpp` - 语法渲染

## 未来增强

### 计划功能

| 功能 | 优先级 | 复杂度 |
|-----|--------|--------|
| 多文件标签 | 高 | 中 |
| 搜索和替换 | 高 | 低 |
| Git 集成 | 中 | 高 |
| 插件系统 | 中 | 高 |
| 主题 | 低 | 低 |
| 分屏窗口 | 低 | 中 |
| Unicode 支持 | 低 | 高 |

### 架构改进

| 改进 | 优先级 | 好处 |
|-----|--------|------|
| 事件系统 | 中 | 更好的解耦 |
| 命令注册表 | 中 | 可扩展性 |
| 异步操作 | 低 | 性能 |
| 配置管理器 | 低 | 集中化 |

## 调试

### 日志文件

- 位置: `editer.log`
- 级别: DEBUG, INFO, WARNING, ERROR
- 带时间戳的条目

### 调试构建

```bash
# 在 build_msvc.bat 中添加调试标志
/Zi /Od /DEBUG
```

### 常见问题

1. **布局问题** - 检查 `editer.log` 中的布局计算
2. **AI 不工作** - 检查 API 配置和远端服务状态
3. **语法高亮** - 检查语言检测和高亮器

## 结论

Editer 的架构强调:
- **模块化** - 清晰的组件边界
- **可维护性** - 组织良好的代码结构
- **可扩展性** - 易于添加新功能
- **性能** - 高效的渲染和操作
- **可靠性** - 全面的错误处理和日志记录

该设计允许未来增长，同时保持代码质量和用户体验。

---

*安装说明请参见 [INSTALL.md](INSTALL.md)*  
*使用指南请参见 [README.md](README.md)*
