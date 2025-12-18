# Editer 测试文档

## 概述

Editer 项目包含一套完整的单元测试框架，用于验证核心编辑功能和编辑器操作的正确性。测试框架基于自定义的轻量级测试框架 `test_framework.hpp`。

## 测试框架

### 测试框架结构

**文件位置**: `test/`

**主要文件**:
- `test_framework.hpp` - 自定义测试框架（宏和断言）
- `test_main.cpp` - 测试入口和运行器
- `test_core.cpp` - 核心功能测试集合

### 测试框架特性

- ✅ **轻量级设计** - 无外部依赖（不依赖 GTest 等）
- ✅ **自动注册** - 使用 `EDITER_TEST()` 宏自动注册测试
- ✅ **异常处理** - 捕获并报告测试异常
- ✅ **多种断言** - 提供 `ASSERT_TRUE` 和 `ASSERT_EQ` 等断言

### 核心宏

```cpp
// 定义测试
EDITER_TEST(TestName) {
    // 测试代码
}

// 断言
ASSERT_TRUE(condition);           // 断言条件为真
ASSERT_EQ(expected, actual);      // 断言两值相等
```

## 测试套件详解

### 1. Buffer 单元测试

**模块**: `core/buffer.hpp`

**测试目标**: 验证文本缓冲区的基础操作

#### 1.1 Buffer_Insert_and_Size

**测试内容**:
```
✓ 在指定位置插入字符
✓ 验证缓冲区大小更新
✓ 验证行内容正确
```

**测试步骤**:
1. 创建空 Buffer
2. 在 (0,0) 位置插入 'A'
3. 在 (0,1) 位置插入 '\n' (换行)
4. 在 (1,0) 位置插入 'B'
5. 验证缓冲区大小为 2 行
6. 验证第 0 行内容为 "A"
7. 验证第 1 行内容为 "B"

**覆盖功能**:
- `Buffer::insert(Position, char)` - 字符插入
- `Buffer::size()` - 获取行数
- `Buffer::line(int)` - 获取行内容

#### 1.2 Buffer_Find

**测试内容**:
```
✓ 在文本中查找子字符串
✓ 返回正确的位置信息
```

**测试步骤**:
1. 创建 Buffer，插入 "Hi!"
2. 使用 `find("i")` 查找字符
3. 验证找到的位置为 (0, 1)

**覆盖功能**:
- `Buffer::find(std::string)` - 字符串查找

#### 1.3 Buffer_Delete

**测试内容**:
```
✓ 删除指定位置的字符
✓ 验证删除后的内容
```

**测试步骤**:
1. 插入 "Hello"
2. 删除位置 (0, 2) 的字符 (第一个 'l')
3. 验证行内容变为 "Helo"

**覆盖功能**:
- `Buffer::erase(Position)` - 字符删除

#### 1.4 Buffer_MultiLine

**测试内容**:
```
✓ 多行文本管理
✓ 每行内容的独立性
```

**测试步骤**:
1. 创建 3 行文本：
   - 第 0 行: "Line 1"
   - 第 1 行: "Line 2"
   - 第 2 行: "Line 3"
2. 验证缓冲区大小为 3
3. 验证每行内容正确

**覆盖功能**:
- 多行管理
- 行号和列号的二维坐标系统

### 2. Editor 集成测试

**模块**: `core/editor.hpp`

**测试目标**: 验证编辑器的高级功能和状态管理

#### 2.1 Editor_Initialize

**测试内容**:
```
✓ 编辑器初始化状态
✓ 缓冲区初始化
```

**测试步骤**:
1. 创建新 Editor 实例
2. 验证缓冲区已初始化，包含 1 行（空行）

**覆盖功能**:
- `Editor::Editor()` - 编辑器初始化
- `Editor::buffer()` - 获取缓冲区引用

#### 2.2 Editor_InsertAndDisplay

**测试内容**:
```
✓ 通过编辑器插入字符
✓ 验证缓冲区内容更新
```

**测试步骤**:
1. 创建 Editor
2. 使用 `insert_char('H')` 插入字符
3. 使用 `insert_char('i')` 插入字符
4. 验证缓冲区内容为 "Hi"

**覆盖功能**:
- `Editor::insert_char(char)` - 字符插入（包括光标移动）
- `Editor::buffer()` - 访问底层缓冲区

#### 2.3 Editor_Undo

**测试内容**:
```
✓ 撤销单个操作
✓ 撤销多个操作
✓ 恢复到初始状态
```

**测试步骤**:
1. 创建 Editor
2. 插入 'X' 和 'Y'（内容为 "XY"）
3. 第一次撤销，验证内容为 "X"
4. 第二次撤销，验证内容为空（初始状态）

**覆盖功能**:
- `Editor::insert_char(char)` - 记录操作到撤销栈
- `Editor::undo()` - 撤销最后一个操作
- `Action` 结构体 - 操作记录

#### 2.4 Editor_Redo

**测试内容**:
```
✓ 重做撤销的操作
✓ 恢复正确的状态
```

**测试步骤**:
1. 创建 Editor
2. 插入 'A'
3. 撤销
4. 验证内容为空
5. 重做
6. 验证内容恢复为 "A"

**覆盖功能**:
- `Editor::redo()` - 重做上一个撤销的操作

#### 2.5 Editor_UndoRedoMultiple

**测试内容**:
```
✓ 复杂的撤销/重做序列
✓ 多个操作的栈管理
```

**测试步骤**:
1. 创建 Editor
2. 插入 'a', 'b', 'c'（内容为 "abc"）
3. 验证内容为 "abc"
4. 撤销两次
5. 验证内容为 "a"
6. 重做两次
7. 验证内容恢复为 "abc"

**覆盖功能**:
- `Editor::undo_stack_` - 撤销栈管理
- `Editor::redo_stack_` - 重做栈管理
- 栈的先进后出特性

## 测试覆盖范围

### 已覆盖的功能

| 功能 | 测试数量 | 覆盖率 |
|------|--------|--------|
| Buffer 基础操作 | 4 个 | ✅ 高 |
| Editor 初始化 | 1 个 | ✅ 高 |
| 字符插入 | 2 个 | ✅ 高 |
| 字符删除 | 1 个 | ✅ 高 |
| 撤销/重做 | 3 个 | ✅ 高 |
| 多行文本 | 1 个 | ✅ 中 |
| **总计** | **9 个** | **✅** |

### 未覆盖的功能
 
 - ❌ Renderer (渲染器) - 需要 UI 测试框架
 - ❌ FileTree (文件树) - 需要模拟文件系统
 - ❌ Terminal (终端) - 需要命令执行模拟
 - ❌ AIChat (AI 聊天) - 需要模拟远端 AI HTTP 服务
 - ❌ 语法高亮 - 需要集成测试
 - ❌ 文件 I/O - 需要文件系统隔离

## 运行测试

### 构建和运行

```powershell
cd f:\code\c++\editer\test
.\build_tests.bat
```

### 测试输出

成功的测试输出：
```
========================================
    Editer Test Suite
========================================

[PASS] Buffer_Insert_and_Size
[PASS] Buffer_Find
[PASS] Buffer_Delete
[PASS] Buffer_MultiLine
[PASS] Editor_Initialize
[PASS] Editor_InsertAndDisplay
[PASS] Editor_Undo
[PASS] Editor_Redo
[PASS] Editor_UndoRedoMultiple

Summary: 9 / 9 passed
Total time: 5 ms

[SUCCESS] All tests passed!
```

### 失败的测试

如果测试失败，会显示：
```
[FAIL] Editor_Undo -> ASSERT_EQ failed: editor.buffer().line(0) == "X" | expect="X" actual=""
```

## 测试框架使用指南

### 添加新测试

```cpp
// 在 test_core.cpp 中添加新测试
EDITER_TEST(YourTestName) {
    // 设置
    Buffer buf;
    
    // 执行
    buf.insert({0, 0}, 'A');
    
    // 验证
    ASSERT_EQ(buf.line(0), std::string("A"));
}
```

### 使用断言

```cpp
// 值相等断言
ASSERT_EQ(actual, expected);

// 布尔条件断言
ASSERT_TRUE(condition);

// 断言会自动输出调试信息
ASSERT_EQ(buf.size(), 3);  // 如果失败，输出：expect=3 actual=2
```

## CMake 配置

**文件**: `test/CMakeLists.txt`

**配置**:
```cmake
cmake_minimum_required(VERSION 3.16)
project(editer_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(TEST_SOURCES
    test_main.cpp
    test_core.cpp
)

add_executable(editer_tests ${TEST_SOURCES})
target_include_directories(editer_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)

enable_testing()
add_test(NAME EditerTests COMMAND editer_tests)
```

## 构建脚本

**文件**: `test/build_tests.bat`

**功能**:
1. 初始化 MSVC 环境（vcvars64.bat）
2. 运行 CMake 配置
3. 编译测试可执行文件
4. 使用 ctest 运行测试
5. 显示测试结果和耗时

## 最佳实践

### 测试编写建议

1. ✅ **单一职责** - 每个测试只测试一个功能
2. ✅ **清晰命名** - 测试名称要说明测试内容
3. ✅ **独立性** - 测试之间不应有依赖关系
4. ✅ **快速执行** - 单元测试应该快速完成
5. ✅ **完整性** - 测试成功和失败两种情况

### 断言建议

```cpp
// ✅ 好的做法
ASSERT_EQ(result, expected);
ASSERT_TRUE(!buffer.empty());

// ❌ 避免
ASSERT_TRUE(result == expected);  // 不如 ASSERT_EQ 清晰
```

## 扩展测试

### 建议添加的测试

1. **文件操作**
   - `Editor_LoadFile` - 加载文件
   - `Editor_SaveFile` - 保存文件
   - `Editor_FileModified` - 文件修改标记

2. **高级编辑**
   - `Editor_ReplaceText` - 替换文本
   - `Editor_CopyPaste` - 复制粘贴
   - `Editor_SelectText` - 文本选择

3. **命令执行**
   - `Editor_ExecCommand` - 执行编辑器命令
   - `Editor_SearchReplace` - 查找替换

4. **性能测试**
   - `Buffer_LargeFile` - 大文件处理
   - `Buffer_Performance` - 性能基准测试

## 参考

- [test_framework.hpp](../test/test_framework.hpp) - 测试框架源码
- [test_core.cpp](../test/test_core.cpp) - 测试实现
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 系统架构

## 结果
![alt text](image.png)
