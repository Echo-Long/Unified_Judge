#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <memory>
#include <map>
#include <utility>
#include <cstdint>

// 跨平台宏定义（自动适配Windows/Linux/macOS）
#ifdef _WIN32
#define PATH_SEP "\\"                 // 修复：Windows使用原生\分隔符，避免cmd解析歧义
#define EXE_SUFFIX ".exe"             // Windows可执行文件后缀
#define DEL_CMD "del /q "             // Windows删除文件命令（静默删除）
#define WEXITSTATUS(stat) (stat)      // Windows进程退出码直接使用
#define FIND_HANDLE intptr_t          // Windows查找句柄类型
#include <io.h>                       // Windows目录遍历头文件
#include <process.h>                  // 修复：Windows的_pclose/_popen头文件，替换unix的pclose/popen
#else
#define PATH_SEP "/"                  // Linux/macOS路径分隔符
#define EXE_SUFFIX ".out"             // Linux/macOS可执行文件后缀
#define DEL_CMD "rm -f "              // Linux/macOS删除文件命令
#define WEXITSTATUS(stat) (WIFEXITED(stat) ? ::WEXITSTATUS(stat) : -1) // Linux获取有效退出码
#define FIND_HANDLE DIR*              // Linux目录遍历句柄类型
#include <dirent.h>                   // Linux/macOS目录遍历头文件
#include <sys/wait.h>                 // Linux waitpid头文件
#endif

// 评测机核心配置（用户可直接修改默认值，也可通过命令行参数自定义）
struct JudgeConfig {
    std::string testcase_dir = "testcase"; // 测试用例目录（默认testcase，新增可通过命令行修改）
    std::string input_prefix = "input";    // 输入文件前缀，默认inputx.txt
    std::string output_prefix = "output";  // 标准输出前缀，默认outputx.txt
    std::string src_file = "main.cpp";     // 被测C++代码文件，默认main.cpp
    std::string exe_file = "judge_temp" EXE_SUFFIX; // 临时编译产物，自动清理
    std::string err_prefix = "error";      // 错误输出文件前缀，默认errorx.txt
    std::string file_suffix = "txt";       // 用例文件后缀，默认txt
};

// 用例运行结果（强类型枚举，C++11类型安全，避免值冲突）
enum class CaseResult {
    SUCCESS,        // 答案正确
    COMPILE_ERROR,  // 编译失败
    RUN_ERROR,      // 运行崩溃/返回码非0
    FILE_MISS,      // 输入/标准输出文件缺失
    ANSWER_ERROR    // 答案不一致
};

// 工具函数：路径拼接（跨平台，自动处理分隔符，无需手动加/或\）
std::string path_join(const std::string& parent, const std::string& child) {
    if (parent.empty() || child.empty()) return parent + child;
    std::string res = parent;
    if (res.back() != PATH_SEP[0]) res += PATH_SEP;
    return res + child;
}

// 工具函数：判断文件/目录是否存在（跨平台）
bool file_exists(const std::string& path) {
    struct stat file_stat;
    return ::stat(path.c_str(), &file_stat) == 0;
}

// 工具函数：判断路径是否为目录（跨平台）
bool is_directory(const std::string& path) {
    struct stat file_stat;
    if (::stat(path.c_str(), &file_stat) != 0) return false;
#ifdef _WIN32
    return (file_stat.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(file_stat.st_mode);
#endif
}

// 工具函数：读取文件所有行（自动去除\r，统一\n换行符，避免跨平台比对误判）
std::vector<std::string> read_file_lines(const std::string& file_path) {
    std::vector<std::string> lines;
    std::ifstream fin(file_path);
    if (!fin.is_open()) return lines;

    std::string line;
    while (std::getline(fin, line)) {
        // 移除Windows独有的\r，仅保留\n，统一跨平台换行符
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(line);
    }
    fin.close();
    return lines;
}

// 工具函数：写入行数据到文件（跨平台，自动加\n）
void write_file(const std::string& file_path, const std::vector<std::string>& lines) {
    std::ofstream fout(file_path);
    if (!fout.is_open()) {
        std::cerr << "[File Error] Failed to write file: " << file_path << std::endl;
        return;
    }
    for (const auto& line : lines) {
        fout << line << std::endl;
    }
    fout.close();
}

// 工具函数：执行系统命令（跨平台），返回退出码，捕获标准输出/错误
// 修复：解决双重pclose、Windows引号解析、跨平台popen/_popen问题
int exec_system_cmd(const std::string& cmd, std::string& err_msg) {
    err_msg.clear();
    std::string final_cmd = cmd + " 2>&1"; // 将标准错误重定向到标准输出，统一捕获
#ifdef _WIN32
    // Windows下：外层包裹双引号，解决多引号命令解析失败；使用_popen（Windows原生）
    final_cmd = "\"" + final_cmd + "\"";
    auto popen_func = &_popen;
    auto pclose_func = &_pclose;
#else
    // Linux/macOS下：使用原生popen/pclose，无需外层引号
    auto popen_func = &popen;
    auto pclose_func = &pclose;
#endif

    // C++11智能指针管理FILE*，自动调用pclose/_pclose释放，避免资源泄漏
    std::unique_ptr<FILE, decltype(pclose_func)> pipe(popen_func(final_cmd.c_str(), "r"), pclose_func);
    if (!pipe) {
        err_msg = "System command execution failed: " + cmd;
        return -1;
    }

    char buf[1024] = {0};
    while (::fgets(buf, sizeof(buf), pipe.get()) != nullptr) {
        err_msg += buf;
        ::memset(buf, 0, sizeof(buf));
    }

    // 修复：移除双重pclose！智能指针会自动调用pclose_func，直接获取退出码即可
    int exit_code = WEXITSTATUS(pclose_func(pipe.get()));
    return exit_code;
}

// 核心函数：逐行比对两个文件，返回是否一致，输出详细差异信息
bool compare_files(const std::string& program_out, const std::string& standard_out, std::string& diff_info) {
    diff_info.clear();
    auto prog_lines = read_file_lines(program_out);
    auto std_lines = read_file_lines(standard_out);

    // 先判断行数是否一致
    if (prog_lines.size() != std_lines.size()) {
        diff_info += "Line count mismatch: Program output " + std::to_string(prog_lines.size()) 
                    + " lines, Standard output " + std::to_string(std_lines.size()) + " lines\n";
    }

    // 逐行比对，记录不一致的行
    size_t min_line_count = std::min(prog_lines.size(), std_lines.size());
    for (size_t i = 0; i < min_line_count; ++i) {
        if (prog_lines[i] != std_lines[i]) {
            diff_info += "Line " + std::to_string(i + 1) + " mismatch:\n";
            diff_info += "  Program output: " + prog_lines[i] + "\n";
            diff_info += "  Standard output: " + std_lines[i] + "\n";
        }
    }

    return diff_info.empty();
}

// 核心函数：遍历测试用例目录，提取所有合法用例编号（按数字升序，自动去重）
std::vector<int> get_testcase_numbers(const JudgeConfig& cfg) {
    std::unordered_set<int> num_set; // 去重，避免重复用例
    // 正则表达式：匹配 前缀+数字+.后缀（如input123.txt）
    std::regex num_regex(cfg.input_prefix + R"((\d+)\.)" + cfg.file_suffix);
    std::string search_dir = cfg.testcase_dir;

#ifdef _WIN32
    // Windows目录遍历逻辑
    std::string search_pattern = path_join(search_dir, cfg.input_prefix + "*." + cfg.file_suffix);
    _finddata_t file_info;
    FIND_HANDLE h_file = _findfirst(search_pattern.c_str(), &file_info);
    if (h_file == -1) return {};

    do {
        std::string file_name = file_info.name;
        std::smatch match;
        if (std::regex_search(file_name, match, num_regex) && match.size() >= 2) {
            num_set.insert(std::stoi(match[1].str()));
        }
    } while (_findnext(h_file, &file_info) == 0);
    _findclose(h_file);
#else
    // Linux/macOS目录遍历逻辑
    FIND_HANDLE dir_ptr = opendir(search_dir.c_str());
    if (!dir_ptr) return {};

    dirent* dir_entry = nullptr;
    while ((dir_entry = readdir(dir_ptr)) != nullptr) {
        std::string file_name = dir_entry->d_name;
        std::smatch match;
        if (std::regex_search(file_name, match, num_regex) && match.size() >= 2) {
            num_set.insert(std::stoi(match[1].str()));
        }
    }
    closedir(dir_ptr);
#endif

    // 转成有序数组（按数字升序执行用例，结果更清晰）
    std::vector<int> case_nums(num_set.begin(), num_set.end());
    std::sort(case_nums.begin(), case_nums.end());
    return case_nums;
}

// 核心函数：编译被测C++代码，返回编译结果
CaseResult compile_source_code(const JudgeConfig& cfg, std::string& error_msg) {
    error_msg.clear();
    // 检查被测代码文件是否存在
    if (!file_exists(cfg.src_file)) {
        error_msg = "Source file not found: " + cfg.src_file + " (Please ensure it's in the current directory)";
        return CaseResult::FILE_MISS;
    }

    // 编译命令：g++ C++11标准 + 源文件 + 输出可执行文件 + 显示所有警告
    std::string compile_cmd = "g++ -std=c++11 \"" + cfg.src_file + "\" -o \"" + cfg.exe_file + "\" -Wall";
    std::string compile_err;
    int ret_code = exec_system_cmd(compile_cmd, compile_err);

    // 编译返回码非0表示编译失败
    if (ret_code != 0) {
        error_msg = "Compile failed:\n" + compile_err;
        return CaseResult::COMPILE_ERROR;
    }

    // 检查编译产物是否生成
    if (!file_exists(cfg.exe_file)) {
        error_msg = "Compile command executed, but no executable file generated: " + cfg.exe_file;
        return CaseResult::RUN_ERROR;
    }

    return CaseResult::SUCCESS;
}

// 核心函数：运行单个测试用例，比对结果，返回运行结果
CaseResult run_single_testcase(int case_num, const JudgeConfig& cfg, std::string& error_info) {
    error_info.clear();
    // 拼接各文件路径（全部适配自定义的测试用例文件夹）
    std::string input_path = path_join(cfg.testcase_dir, cfg.input_prefix + std::to_string(case_num) + "." + cfg.file_suffix);
    std::string std_out_path = path_join(cfg.testcase_dir, cfg.output_prefix + std::to_string(case_num) + "." + cfg.file_suffix);
    std::string temp_out_path = path_join(cfg.testcase_dir, "temp_" + std::to_string(case_num) + "." + cfg.file_suffix);
    std::string err_out_path = path_join(cfg.testcase_dir, cfg.err_prefix + std::to_string(case_num) + "." + cfg.file_suffix);

    // 检查输入/标准输出文件是否存在
    if (!file_exists(input_path)) {
        error_info = "Input file missing: " + input_path;
        return CaseResult::FILE_MISS;
    }
    if (!file_exists(std_out_path)) {
        error_info = "Standard output file missing: " + std_out_path;
        return CaseResult::FILE_MISS;
    }

    // 运行命令：重定向输入（<）和输出（>），跨平台兼容
    std::string run_cmd;
#ifdef _WIN32
    // 修复：Windows下可执行文件无需额外路径（当前目录），引号包裹所有路径
    run_cmd = "\"" + cfg.exe_file + "\" < \"" + input_path + "\" > \"" + temp_out_path + "\"";
#else
    // Linux/macOS下需要./指定当前目录可执行文件
    run_cmd = "./" + cfg.exe_file + " < " + input_path + " > " + temp_out_path;
#endif

    // 执行运行命令，捕获运行错误
    std::string run_err;
    int ret_code = exec_system_cmd(run_cmd, run_err);

    // 运行返回码非0表示运行崩溃/异常
    if (ret_code != 0) {
        error_info = "Run error (Exit code: " + std::to_string(ret_code) + "):\n" + run_err;
        exec_system_cmd(DEL_CMD + temp_out_path, run_err); // 清理临时文件
        return CaseResult::RUN_ERROR;
    }

    // 检查程序是否生成输出文件
    if (!file_exists(temp_out_path)) {
        error_info = "Program ran without exception, but no output file generated";
        return CaseResult::RUN_ERROR;
    }

    // 比对程序输出和标准输出
    std::string diff_info;
    bool is_correct = compare_files(temp_out_path, std_out_path, diff_info);
    if (!is_correct) {
        // 答案错误，将临时输出重命名为错误文件（留存供排查）
        std::string mv_cmd;
#ifdef _WIN32
        // Windows rename命令：路径带空格需引号，且不支持跨目录
        mv_cmd = "rename \"" + temp_out_path + "\" \"" + cfg.err_prefix + std::to_string(case_num) + "." + cfg.file_suffix + "\"";
#else
        mv_cmd = "mv " + temp_out_path + " " + err_out_path;
#endif
        exec_system_cmd(mv_cmd, run_err);
        error_info = "Answer error:\n" + diff_info + "Error output saved to: " + err_out_path;
        return CaseResult::ANSWER_ERROR;
    }

    // 答案正确，清理临时输出文件
    exec_system_cmd(DEL_CMD + temp_out_path, run_err);
    return CaseResult::SUCCESS;
}

// 辅助函数：打印帮助信息（新增-t参数说明，全英文无乱码）
void print_help_info(const char* program_name) {
    std::cout << "==================== C++11 Cross-Platform Judge ====================" << std::endl;
    std::cout << "Usage: " << program_name << " [optional arguments]" << std::endl;
    std::cout << "Optional arguments (space required between option and value):" << std::endl;
    std::cout << "  -t <dir>      Custom testcase directory (default: testcase)" << std::endl; // 新增-t参数
    std::cout << "  -i <prefix>   Custom input file prefix (default: input, e.g.: input1.txt)" << std::endl;
    std::cout << "  -o <prefix>   Custom standard output prefix (default: output, e.g.: output1.txt)" << std::endl;
    std::cout << "  -c <file>     Custom tested C++ source file (default: main.cpp)" << std::endl;
    std::cout << "  -h            Print this help information" << std::endl;
    std::cout << "====================================================================" << std::endl;
    std::cout << "Example 1: " << program_name << " -t mytest -i in -o ans -c sol.cpp" << std::endl; // 新增示例
    std::cout << "Desc 1: Match mytest/inx.txt & mytest/ansx.txt, test sol.cpp" << std::endl;
    std::cout << "Example 2: " << program_name << " -t cases -c main.cpp" << std::endl;
    std::cout << "Desc 2: Match cases/inputx.txt & cases/outputx.txt, test main.cpp" << std::endl;
    std::cout << "====================================================================" << std::endl;
}

// 核心函数：纯C++11手动解析命令行参数（新增-t参数解析，跨平台无依赖）
void parse_command_args(int argc, char* argv[], JudgeConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // 匹配参数，确保后续有值（避免数组越界）
        if ((arg == "-t" || arg == "--testcase") && i + 1 < argc) { // 新增-t参数解析
            cfg.testcase_dir = argv[++i];
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            cfg.input_prefix = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            cfg.output_prefix = argv[++i];
        } else if ((arg == "-c" || arg == "--code") && i + 1 < argc) {
            cfg.src_file = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            print_help_info(argv[0]);
            exit(0);
        } else {
            // 无效参数，打印帮助并退出
            std::cerr << "[Arg Error] Invalid argument: " << arg << std::endl;
            print_help_info(argv[0]);
            exit(1);
        }
    }
}

// 主函数：评测机入口，串联所有逻辑（无需修改，已适配自定义文件夹）
int main(int argc, char* argv[]) {
    std::cout << "==================== C++11 Cross-Platform Judge ====================" << std::endl;
    JudgeConfig cfg;
    // 解析命令行参数（含新增的-t参数）
    parse_command_args(argc, argv, cfg);

    // 检查测试用例目录是否存在（致命错误，不存在则退出，适配自定义文件夹）
    if (!file_exists(cfg.testcase_dir) || !is_directory(cfg.testcase_dir)) {
        std::cerr << "[Fatal Error] Testcase directory not found!" << std::endl;
        std::cerr << "Please create a '" << cfg.testcase_dir << "' subfolder in current directory, and put inputx.txt/outputx.txt" << std::endl;
        return 1;
    }

    // 步骤1：编译被测代码
    std::cout << "\n[Step 1] Compiling tested source code: " << cfg.src_file << std::endl;
    std::string compile_error;
    CaseResult compile_res = compile_source_code(cfg, compile_error);
    if (compile_res != CaseResult::SUCCESS) {
        std::cerr << "" << compile_error << std::endl;
        return 1;
    }
    std::cout << " Compile success!" << std::endl;

    // 步骤2：提取所有测试用例编号（适配自定义文件夹）
    std::cout << "\n[Step 2] Traversing testcase directory: " << cfg.testcase_dir << std::endl;
    std::vector<int> case_nums = get_testcase_numbers(cfg);
    if (case_nums.empty()) {
        std::cerr << "️No valid testcases found (Format: " << cfg.input_prefix << "x." << cfg.file_suffix << " in " << cfg.testcase_dir << ")" << std::endl;
        // 清理临时可执行文件
        std::string del_err;
        exec_system_cmd(DEL_CMD + cfg.exe_file, del_err);
        return 0;
    }
    std::cout << "Found " << case_nums.size() << " valid testcases, numbers: ";
    for (int num : case_nums) std::cout << num << " ";
    std::cout << std::endl;

    // 步骤3：逐个运行测试用例并比对（适配自定义文件夹）
    std::cout << "\n[Step 3] Starting testcase execution..." << std::endl;
    int success_count = 0, fail_count = 0;
    // 记录失败用例（map替代unordered_map，全编译器兼容）
    std::map<int, std::pair<CaseResult, std::string>> fail_cases;

    for (int case_num : case_nums) {
        std::cout << "\n---------------------- Testcase " << case_num << " ----------------------" << std::endl;
        std::string case_error;
        CaseResult case_res = run_single_testcase(case_num, cfg, case_error);
        if (case_res == CaseResult::SUCCESS) {
            std::cout << "Testcase " << case_num << ": Answer correct!" << std::endl;
            success_count++;
        } else {
            std::cerr << "Testcase " << case_num << ": " << case_error << std::endl;
            fail_count++;
            fail_cases[case_num] = {case_res, case_error};
        }
    }

    // 步骤4：输出最终评测报告（适配自定义文件夹）
    std::cout << "\n==================== Judge Report ====================" << std::endl;
    std::cout << "Total testcases: " << case_nums.size() << std::endl;
    std::cout << "Passed testcases: " << success_count << std::endl;
    std::cout << "Failed testcases: " << fail_count << std::endl;
    if (fail_count > 0) {
        std::cout << "Failed testcase numbers: ";
        for (const auto& pair : fail_cases) std::cout << pair.first << " ";
        std::cout << std::endl;
        std::cout << "Error output files: " << cfg.testcase_dir << PATH_SEP << cfg.err_prefix << "x." << cfg.file_suffix << std::endl;
    } else {
        std::cout << "All testcases passed!" << std::endl;
    }
    std::cout << "======================================================" << std::endl;

    // 清理临时可执行文件（无论成功失败，都清理）
    std::string del_error;
    int del_ret = exec_system_cmd(DEL_CMD + cfg.exe_file, del_error);
    if (del_ret != 0 && !del_error.empty()) {
        std::cerr << "Warning: Failed to clean temporary file: " << del_error << std::endl;
    }

    // 失败则返回非0码，方便脚本调用
    return fail_count > 0 ? 1 : 0;
}