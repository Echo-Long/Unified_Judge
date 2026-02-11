# Unified Judge
跨语言自动化评测工具，统一适配 C++ / Python 代码的自动化评测流程，可批量验证代码输出是否符合测试用例预期，整合原 C++/Python 评测器的核心功能并补充完善跨平台、错误检测、输出比对等特性。

## 核心特性
- 🚀 **跨语言支持**：同时适配编译型语言（C++）和解释型语言（Python）的评测流程
- 🎯 **灵活的用例匹配**：支持数字编号（`input1.txt`/`output1.txt`）、自定义标识（`input_test1.txt`/`output_test1.txt`）等多种测试用例命名格式
- 📝 **完善的输出比对**：自动处理换行符差异（移除 `\r`）、忽略行尾空格，精准提示行数不一致、内容不匹配、多余/缺失行等问题
- ⚠️ **多维度错误检测**：覆盖编译错误（C++）、运行超时、脚本语法/运行时错误（Python）、退出码异常、文件缺失等场景
- 🌍 **跨平台兼容**：适配 Windows / Linux / macOS 系统，自动兼容不同系统的命令行语法
- 🛠️ **易用性优化**：简洁的命令行参数、可视化评测报告、批量清理临时/错误文件
- 📊 **详细报告**：输出通过率、失败用例详情、错误原因，便于快速定位问题

## 环境要求
### 基础环境
- Python 3.6+（推荐 3.8+）：核心运行环境，Windows/Linux/macOS 均需安装
  - Windows：推荐从 [Python 官网](https://www.python.org/downloads/) 安装，勾选「Add Python to PATH」
  - Linux/macOS：系统通常自带 Python3，若未安装可执行 `sudo apt install python3`（Linux）/ `brew install python3`（macOS）

### 可选环境（仅评测 C++ 时需要）
- C++ 编译器（g++）：
  - Windows：安装 [MinGW](https://sourceforge.net/projects/mingw-w64/)，并将 `mingw64/bin` 加入系统环境变量
  - Linux：`sudo apt install g++`
  - macOS：`xcode-select --install` 或 `brew install gcc`

## 快速使用
### 1. 准备工作
- 将待评测的代码文件（如 `main.cpp`/`main.py`）放在任意目录
- 准备测试用例：在指定目录下创建 `inputXXX.txt`（输入用例）和 `outputXXX.txt`（预期输出），例如：
  ```
  testcases/
  ├── input1.txt       # 测试用例1输入
  ├── output1.txt      # 测试用例1预期输出
  ├── input_test2.txt  # 测试用例2输入（自定义标识）
  └── output_test2.txt # 测试用例2预期输出
  ```

### 2. 基础命令示例
#### 评测 C++ 代码
```bash
python unified_judge.py -l cpp -s main.cpp -t ./testcases -to 10
```

#### 评测 Python 脚本
```bash
python unified_judge.py -l python -s main.py -t ./testcases -clean
```

## 命令行参数详解
Unified Judge 提供丰富的命令行参数，支持灵活配置评测流程，所有参数均可通过 `python unified_judge.py -h` 查看完整说明。

### 参数总览表
| 参数名（长格式） | 缩写 | 是否必选 | 数据类型 | 默认值 | 取值范围 | 核心作用 |
|------------------|------|----------|----------|--------|----------|----------|
| `--lang`         | `-l` | 是       | 字符串   | 无     | `cpp`/`python` | 指定待评测代码的编程语言，决定评测流程（编译+运行/直接运行） |
| `--src-file`     | `-s` | 否       | 字符串   | Windows：`main.cpp`；Linux/macOS：`main.py` | 合法文件路径 | 指定待评测的代码/脚本文件路径（支持相对/绝对路径） |
| `--testcase-dir` | `-t` | 否       | 字符串   | 当前工作目录（`.`） | 合法目录路径 | 指定测试用例所在目录，工具会自动扫描该目录下的输入/输出用例对 |
| `--timeout`      | `-to`| 否       | 整数     | 30     | ≥1       | 设置编译（仅C++）、运行阶段的超时时间（单位：秒），超时则判定为运行错误 |
| `--clean-temp`   | `-clean` | 否    | 布尔     | False  | 无（仅开关） | 评测完成后自动清理临时文件（`temp_*.txt`）和错误输出文件（`error_*.txt`） |

### 各参数详细说明
#### 1. 核心必选参数：`--lang` / `-l`
- **作用**：指定待评测代码的编程语言，工具会根据该参数选择对应的评测逻辑：
  - `cpp`：启用「编译→运行→比对」流程，适配C++代码（会先调用g++编译代码，再运行生成的可执行文件）
  - `python`：启用「直接运行→比对」流程，适配Python脚本（直接调用Python解释器运行脚本）
- **使用示例**：
  ```bash
  # 评测C++代码
  python unified_judge.py -l cpp -s main.cpp
  # 评测Python脚本
  python unified_judge.py -l python -s main.py
  ```
- **注意事项**：
  - 该参数无默认值，必须显式指定，否则工具会直接报错并打印帮助信息
  - 输入值需严格为小写的 `cpp` 或 `python`，不支持大写（如 `CPP`/`Python`）或缩写（如 `c++`）

#### 2. 代码文件参数：`--src-file` / `-s`
- **作用**：指定待评测的代码文件路径，支持相对路径（如 `./src/main.cpp`）和绝对路径（如 `D:/code/main.py`）
- **默认值**：
  - Windows系统：默认查找当前目录下的 `main.cpp`
  - Linux/macOS系统：默认查找当前目录下的 `main.py`
- **使用示例**：
  ```bash
  # 相对路径（推荐，适配跨平台）
  python unified_judge.py -l cpp -s ./src/solution.cpp -t ./testcases
  # 绝对路径（Windows）
  python unified_judge.py -l python -s D:/projects/leetcode/solution.py
  # 绝对路径（Linux/macOS）
  python unified_judge.py -l python -s /home/user/code/solution.py
  ```
- **注意事项**：
  - 文件路径中尽量避免中文、空格或特殊字符（如 `!@#$%`），否则可能导致文件找不到或执行失败
  - 确保文件存在且可读取，否则工具会提示「File not found」并判定为文件缺失错误

#### 3. 测试用例目录参数：`--testcase-dir` / `-t`
- **作用**：指定测试用例所在的目录，工具会自动扫描该目录下所有符合命名规则的输入/输出用例对
- **默认值**：当前工作目录（即执行命令时的目录，用 `.` 表示）
- **使用示例**：
  ```bash
  # 测试用例在当前目录的testcases子目录
  python unified_judge.py -l cpp -s main.cpp -t ./testcases
  # 测试用例在上级目录的cases目录（Linux/macOS）
  python unified_judge.py -l python -s main.py -t ../cases
  # 测试用例在绝对路径（Windows）
  python unified_judge.py -l cpp -s main.cpp -t D:/testcases/week1
  ```
- **注意事项**：
  - 目录路径同样避免中文、空格或特殊字符
  - 若指定的目录不存在，工具会直接报错并退出
  - 工具仅扫描该目录下的文件，不会递归扫描子目录

#### 4. 超时时间参数：`--timeout` / `-to`
- **作用**：设置编译（仅C++）和运行阶段的最大耗时，超过该时间则判定为「运行错误（超时）」
- **默认值**：30（单位：秒）
- **取值范围**：≥1的整数（建议不小于1秒，否则可能误判简单代码）
- **使用示例**：
  ```bash
  # 设置超时时间为10秒（适合简单算法/小数据量测试用例）
  python unified_judge.py -l cpp -s main.cpp -to 10
  # 设置超时时间为60秒（适合复杂算法/大数据量测试用例）
  python unified_judge.py -l python -s main.py -to 60
  ```
- **注意事项**：
  - C++的编译阶段和运行阶段均受该超时时间限制（例如设置 `-to 10`，编译超过10秒也会判定为超时）
  - 超时时间需根据代码复杂度合理设置，过短会误判正常代码，过长会增加整体评测耗时

#### 5. 临时文件清理参数：`--clean-temp` / `-clean`
- **作用**：评测完成后自动删除测试用例目录下的临时文件（`temp_*.txt`）和错误输出文件（`error_*.txt`），避免目录杂乱
- **默认值**：False（不清理，保留临时/错误文件）
- **使用方式**：无需赋值，仅需在命令中添加该参数即可启用（属于「开关型参数」）
- **使用示例**：
  ```bash
  # 评测后自动清理临时/错误文件
  python unified_judge.py -l cpp -s main.cpp -t ./testcases -clean
  # 结合超时参数使用
  python unified_judge.py -l python -s main.py -to 15 -clean
  ```
- **注意事项**：
  - 启用该参数后，错误输出文件（`error_*.txt`）会被删除，若需要保留错误输出用于排查问题，请勿启用
  - 仅清理测试用例目录下的 `temp_*` 和 `error_*` 前缀文件，不会影响其他文件（如测试用例本身）

### 常用参数组合示例
以下是实际使用中最常见的参数组合，可直接参考复用：

#### 场景1：快速评测当前目录的C++代码（默认测试用例目录）
```bash
# 仅指定必选的lang，使用默认的src-file（main.cpp）、testcase-dir（.）、timeout（30秒）
python unified_judge.py -l cpp
# 简化版：指定超时时间为10秒，加快评测效率
python unified_judge.py -l cpp -to 10
```

#### 场景2：评测Python脚本，指定测试用例目录并自动清理临时文件
```bash
# 脚本文件在src子目录，测试用例在testcases/leetcode目录，评测后清理临时文件
python unified_judge.py -l python -s ./src/solution.py -t ./testcases/leetcode -clean
```

#### 场景3：评测复杂C++算法，延长超时时间并指定绝对路径
```bash
# 代码文件和测试用例均使用绝对路径，超时时间设为60秒适配复杂算法
python unified_judge.py -l cpp -s D:/code/algorithm.cpp -t D:/testcases/big_data -to 60
```

#### 场景4：全参数自定义（生产环境推荐）
```bash
# 明确指定所有参数，避免依赖默认值导致的问题
python unified_judge.py -l python \
  -s /home/user/code/leetcode/1_two_sum.py \
  -t /home/user/testcases/leetcode/1_two_sum \
  -to 15 \
  -clean
```

### 参数错误处理提示
- 若参数输入错误（如 `-l c++` 而非 `-l cpp`），工具会输出清晰的错误提示并退出，同时打印完整的参数帮助信息
- 若必选参数缺失（如未指定 `-l`），工具会直接打印帮助信息并退出，不会执行评测
- 若参数值不合法（如 `-to 0` 或 `-to abc`），工具会提示合法取值范围并退出
- 若路径类参数（`-s`/`-t`）指向不存在的文件/目录，工具会明确提示「文件/目录不存在」并退出

## 测试用例规范
### 命名规则
- 输入文件必须以 `input` 开头，输出文件必须以 `output` 开头，后缀为 `.txt`/`.in`/`.out`（大小写不敏感）
- 输入/输出文件的「标识部分」需完全一致（大小写不敏感），例如：
  ✅ 合法匹配：`input1.txt` ↔ `output1.txt`、`Input_Test.txt` ↔ `Output_Test.txt`、`input2.in` ↔ `output2.out`
  ❌ 不匹配：`input1.txt` ↔ `output2.txt`、`input_test.txt` ↔ `output_test1.txt`

### 内容规范
- 文本编码推荐使用 **UTF-8**，避免乱码
- 换行符自动兼容（Windows `\r\n`、Linux/macOS `\n`），无需手动转换

## 输出说明
### 评测报告示例
```
[Step 1] Auto scanning testcase directory: ./testcases
Found 2 valid testcase pairs:
  1. input1.txt ↔ output1.txt
  2. input_test2.txt ↔ output_test2.txt

[Step 2] Running testcases...

---------------------- Testcase: 1 ----------------------
✅ 1: Answer correct!

---------------------- Testcase: test2 ----------------------
❌ test2: Answer error:
Line count mismatch: Your output (2) vs Expected (3)
Line 2 mismatch:
  Your output: 5
  Expected: 6

Missing lines in your output (from line 3):
  Line 3: 7
Error output saved to: ./testcases/error_test2.txt

==================== Judge Report ====================
Total testcases: 2
✅ Passed: 1
❌ Failed: 1

Failed testcases details:

[ANSWER_ERROR] test2:
  Answer error:
Line count mismatch: Your output (2) vs Expected (3)
Line 2 mismatch:
  Your output: 5
  Expected: 6

Missing lines in your output (from line 3):
  Line 3: 7
Error output saved to: ./testcases/error_test2.txt

Error output files: error_<identifier>.txt (in ./testcases)
======================================================
```

### 错误文件说明
- 评测失败时，会生成 `error_<标识>.txt` 文件（位于测试用例目录），保存代码实际输出，便于对比排查
- 若启用 `-clean` 参数，评测完成后会自动删除 `temp_*.txt` 和 `error_*.txt`

## 常见问题
### Q1: 评测 C++ 时提示「Compile failed」
- 检查 g++ 是否安装并加入环境变量（执行 `g++ --version` 验证）
- 检查代码是否有语法错误、链接错误（报告中会输出具体编译错误信息）
- 确保代码文件名和路径无中文/空格

### Q2: 测试用例匹配不到（提示「No valid testcase pairs found」）
- 检查测试用例目录路径是否正确（可通过绝对路径验证）
- 检查输入/输出文件命名是否符合规则（`inputXXX` 和 `outputXXX` 的标识部分需完全一致）
- 检查文件后缀是否为 `.txt`/`.in`/`.out`（工具仅扫描这三类后缀）

### Q3: 运行超时（提示「Timeout (>XXs exceeded)」）
- 检查代码是否有死循环、无限递归等导致卡死的逻辑
- 适当增大 `-to` 参数（如 `-to 60`）延长超时时间
- 优化代码性能（如减少循环次数、优化算法时间复杂度）

### Q4: Python 评测提示「Script error」
- 直接运行脚本 `python main.py` 排查语法/运行时错误（工具会输出具体错误信息）
- 检查脚本是否依赖第三方库（需提前通过 `pip install` 安装）
- 确保脚本的执行权限（Linux/macOS 可执行 `chmod +x main.py`）

### Q5: 跨平台运行时路径报错
- 优先使用相对路径（如 `./testcases`）而非绝对路径
- Linux/macOS 路径分隔符用 `/`，Windows 用 `\` 或 `/`（工具已兼容）
- 避免路径中包含中文、空格或特殊字符

## 许可证
本工具为开源自用工具，可自由修改、分发，无商业限制。

## 温馨提示
- 评测前建议先手动运行代码，确保无基础语法/运行错误，减少无效评测
- 测试用例数量较多时，建议启用 `-clean` 参数避免生成大量临时文件
- 跨系统使用时，建议统一使用 UTF-8 编码和相对路径，减少环境兼容问题
- 若需长期使用，可将工具所在目录加入系统环境变量，方便任意目录执行
