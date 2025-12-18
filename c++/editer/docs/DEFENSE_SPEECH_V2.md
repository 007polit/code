# 配置语言解释器的词法分析

## 需求

编辑器解析配置文件 `config.cfg`：
```properties
set tabsize = 2
set auto_save = true
```

将其转换为 Token 序列，再存入配置 map。

---

## 词法分析方案

### 三阶段处理

**第一阶段：trim** - 去除行首尾空白

```cpp
std::string ConfigInterpreter::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    while (start < end && std::isspace(str[start])) start++;
    while (end > start && std::isspace(str[end - 1])) end--;
    return str.substr(start, end - start);
}
```

**第二阶段：tokenize** - 按空白符分割

```cpp
std::vector<std::string> ConfigInterpreter::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}
```

**第三阶段：parse_command** - 语义验证

```cpp
bool ConfigInterpreter::parse_command(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return true;
    if (tokens[0][0] == '#') return true;  // 注释
    
    if (tokens.size() >= 4 && tokens[0] == "set" && tokens[2] == "=") {
        std::string key = tokens[1];
        std::string value = tokens[3];
        for (size_t i = 4; i < tokens.size(); i++) {
            value += " " + tokens[i];  // 多词值合并
        }
        config_[key] = value;
        return true;
    }
    return false;
}
```

**数据结构**：`std::map<std::string, std::string> config_`

**类型转换**：

```cpp
int ConfigInterpreter::get_int(const std::string& key, int default_value) const {
    auto it = config_.find(key);
    if (it == config_.end()) return default_value;
    try { return std::stoi(it->second); }
    catch (...) { return default_value; }
}

bool ConfigInterpreter::get_bool(const std::string& key, bool default_value) const {
    auto it = config_.find(key);
    if (it == config_.end()) return default_value;
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1" || value == "yes";
}
```

---

## 为什么不需要有限自动机（DFA）？

**标准编译器**需要处理复杂的词汇形式（整数、浮点、字符串、多字符符号等），所以需要状态机。

**我们的配置语言**只有：`set key = value`

空白符就足以分隔所有词元，无需前向扫描、符号表或状态机。

代码：**~100 行** vs 标准编译器 **~3000 行**

---

## 关键挑战与解决方案

1. **多词值处理**：从第 4 个 Token 开始逐个合并空格分隔的值
2. **行首注释**：检查 `tokens[0][0] == '#'` 跳过注释行
3. **非法输入**：验证格式失败返回 false，继续下一行（容错）
4. **Windows 路径中的 `+` 号问题**：
   - 原因：cmd.exe 将 `+` 视为特殊字符
   - 解决：为路径添加引号 `"F:\code\c++\editer\test"`
   - 建议：优先使用 PowerShell

---

## 总结

- **核心思想**：根据问题特点选择合适的设计
- **简洁优势**：~100 行代码 vs 标准编译器 ~3000 行
- **配置系统不需要**：DFA、前向扫描、符号表
- **为什么**：空白符天然分隔，格式结构清晰，语句类型单一
