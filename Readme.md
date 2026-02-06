# C++11 Cross-Platform Judge
轻量级**跨平台C++自动评测机**，基于纯C++11实现，无任何第三方依赖，可在Windows/Linux/macOS下运行。核心实现**自动编译被测C++代码、批量执行测试用例、逐行比对输出结果**，并生成详细的评测报告，适合算法作业自测、小型C++项目单元测试、编程竞赛本地调试等场景。

## 🌟 核心特性
- **纯C++11实现**：无外部库依赖，编译即可运行，移植性强
- **跨平台兼容**：完美适配Windows(MinGW)、Linux、macOS，底层自动适配系统命令/路径/目录遍历
- **全流程自动化**：自动编译被测代码 → 自动识别测试用例 → 自动运行+重定向输入输出 → 自动逐行比对结果
- **灵活的自定义配置**：支持命令行参数自定义测试用例目录、输入/输出文件前缀、被测源代码文件
- **详细的错误提示**：编译错误、运行错误、文件缺失、答案错误均给出精准提示，定位问题更高效
- **智能文件管理**：自动清理临时编译产物，答案错误时留存程序输出文件，方便问题排查
- **规范的评测报告**：最终输出用例总数/通过数/失败数，以及失败用例编号，结果一目了然

## 📋 环境要求
### 编译器
需支持**C++11及以上标准**的编译器，推荐：
- Windows：MinGW-w64（GCC）
- Linux：GCC/Clang（系统自带）
- macOS：Clang/GCC（Xcode Command Line Tools 或 Homebrew 安装）

### 核心依赖
被测代码的编译依赖**GCC（g++）**，Windows需将MinGW的`bin`目录加入系统环境变量，Linux/macOS一般自带GCC，可通过`g++ --version`验证是否安装成功。

## 🚀 快速开始
### 1. 克隆仓库
```bash
git clone https://github.com/你的用户名/C++11-Cross-Platform-Judge.git
cd C++11-Cross-Platform-Judge
```

### 2. 编译评测机
#### Windows (MinGW)
```bash
g++ judge.cpp -o judge.exe -std=c++11
```
#### Linux / macOS
```bash
g++ judge.cpp -o judge -std=c++11
```

### 3. 准备测试用例
在当前目录创建**测试用例目录**（默认名`testcase`，可自定义），按**固定命名规则**放入输入/标准输出文件：
- 输入文件：`前缀+数字.后缀`，默认`input1.txt、input2.txt、input3.txt...`
- 标准输出文件：`前缀+数字.后缀`，默认`output1.txt、output2.txt、output3.txt...`
- 命名规则：**数字必须一一对应**（input1.txt 对应 output1.txt），后缀默认`txt`

示例目录结构：
```
C++11-Cross-Platform-Judge/
├── judge.cpp       # 评测机核心代码
├── judge.exe       # Windows编译产物
├── judge           # Linux/macOS编译产物
├── main.cpp        # 你的被测C++代码
└── testcase/       # 测试用例目录
    ├── input1.txt
    ├── output1.txt
    ├── input2.txt
    ├── output2.txt
    └── ...
```

### 4. 运行评测机
将你的被测C++代码（如`main.cpp`）放在评测机同级目录，直接运行即可完成全流程评测。

#### Windows
```bash
# 默认配置：测试main.cpp，使用testcase目录下的inputx.txt/outputx.txt
judge.exe
```
#### Linux / macOS
```bash
# 默认配置：测试main.cpp，使用testcase目录下的inputx.txt/outputx.txt
./judge
```

## 📖 详细使用方法
### 命令行参数
支持**自定义命令行参数**覆盖默认配置，参数与值之间需用空格分隔，所有参数均为可选：
| 参数 | 全称 | 作用 | 默认值 |
|------|------|------|--------|
| `-t` | `--testcase` | 自定义测试用例目录 | `testcase` |
| `-i` | `--input` | 自定义输入文件前缀 | `input` |
| `-o` | `--output` | 自定义标准输出文件前缀 | `output` |
| `-c` | `--code` | 自定义被测C++源代码文件 | `main.cpp` |
| `-h` | `--help` | 打印帮助信息并退出 | - |

### 常用示例
#### 示例1：测试指定被测文件（如`lab1.cpp`）
```bash
# Windows
judge.exe -c lab1.cpp
# Linux/macOS
./judge -c lab1.cpp
```

#### 示例2：自定义测试用例目录（如`mytest`）+ 被测文件（如`solution.cpp`）
```bash
# Windows
judge.exe -t mytest -c solution.cpp
# Linux/macOS
./judge -t mytest -c solution.cpp
```

#### 示例3：自定义输入/输出前缀（如输入`in`、输出`ans`）+ 测试用例目录`cases`
```bash
# Windows：匹配 cases/in1.txt & cases/ans1.txt
judge.exe -t cases -i in -o ans -c main.cpp
# Linux/macOS
./judge -t cases -i in -o ans -c main.cpp
```

#### 示例4：打印帮助信息
```bash
# Windows
judge.exe -h
# Linux/macOS
./judge -h
```

## 📂 目录结构规范
### 测试用例目录要求
1. 测试用例目录需为**当前目录的子目录**（如`testcase`、`mytest`）
2. 输入/标准输出文件需**数字编号一一对应**，后缀统一为`txt`（代码内可直接修改默认后缀）
3. 不限制测试用例数量，程序会**自动识别所有合法编号**并按**数字升序**执行

### 错误输出文件
当测试用例**答案错误**时，程序会将被测代码的输出结果留存为**错误输出文件**，存放在测试用例目录下，命名规则：`error+数字.txt`（如`error1.txt`），方便对比排查问题。

## ⚙️ 直接修改代码默认配置
若需长期使用固定配置，可直接修改`judge.cpp`中的**JudgeConfig结构体**，无需每次输入命令行参数：
```cpp
struct JudgeConfig {
    std::string testcase_dir = "testcase"; // 测试用例目录
    std::string input_prefix = "input";    // 输入文件前缀
    std::string output_prefix = "output";  // 标准输出前缀
    std::string src_file = "main.cpp";     // 被测代码文件
    std::string exe_file = "judge_temp" EXE_SUFFIX; // 临时编译产物
    std::string err_prefix = "error";      // 错误输出前缀
    std::string file_suffix = "txt";       // 用例文件后缀
};
```

## 📊 评测结果说明
程序执行后会输出**分步骤日志**和**最终评测报告**，核心结果说明：
### 编译阶段
- 编译成功：继续执行测试用例
- 编译失败：输出详细的g++编译错误信息，程序退出（需修复被测代码语法错误）

### 用例执行阶段
每个测试用例的执行结果包含5种状态：
1. `SUCCESS`：答案正确，逐行比对完全一致
2. `COMPILE_ERROR`：被测代码编译失败（仅编译阶段触发）
3. `RUN_ERROR`：被测代码运行崩溃/退出码非0（如数组越界、除0错误）
4. `FILE_MISS`：输入文件/标准输出文件缺失（检查文件命名和路径）
5. `ANSWER_ERROR`：答案不一致，留存错误输出文件

### 最终评测报告
包含核心统计信息：
- 总测试用例数
- 通过测试用例数
- 失败测试用例数
- 失败用例编号（若有）
- 错误输出文件路径（若有）

示例最终报告：
```
==================== Judge Report ====================
Total testcases: 4
Passed testcases: 3
Failed testcases: 1
Failed testcase numbers: 2
Error output files: testcase/errorx.txt
======================================================
```

## ❗ 常见问题与解决方案
### 问题1：Windows下提示`'xxx' 不是内部或外部命令`
- 原因：MinGW的`g++`未加入系统环境变量，或命令行解析引号问题
- 解决方案：
  1. 将MinGW的`bin`目录（如`C:\MinGW-w64\mingw64\bin`）加入系统PATH环境变量
  2. 重启命令行工具（CMD/PowerShell）后重新运行

### 问题2：提示`Testcase directory not found!`
- 原因：未创建测试用例目录，或目录名与配置/命令行参数不一致
- 解决方案：在当前目录创建对应名称的子目录（如`testcase`）

### 问题3：提示`No valid testcases found`
- 原因：测试用例目录下无合法命名的输入文件，或文件命名不符合规则
- 解决方案：检查输入文件命名（如`input1.txt`，前缀+数字+后缀），确保无拼写错误

### 问题4：运行错误`Exit code: 1`
- 原因：被测代码运行时触发异常（如数组越界、除0、空指针访问）
- 解决方案：结合被测代码逻辑排查，或通过调试模式运行被测代码定位问题

### 问题5：Linux/macOS下提示`permission denied`
- 原因：评测机可执行文件无执行权限
- 解决方案：执行`chmod +x judge`添加执行权限后重新运行

## 📄 许可证
本项目采用**MIT许可证**开源，可自由修改、分发、商用，无需授权，保留版权声明即可。

## 🤝 贡献
欢迎提交PR完善功能，比如：
- 支持更多文件后缀
- 增加时间限制/内存限制
- 支持多组测试用例批量导入
- 生成HTML/Markdown格式的评测报告
- 适配MSVC编译器

---
### 温馨提示
本项目为**轻量级自测工具**，专注于**本地快速验证**C++代码的正确性，若需工业级评测系统，可在此基础上扩展进程监控、资源限制、多线程执行等功能。
