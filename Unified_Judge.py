import os
import sys
import re
import argparse
import subprocess
import platform
from typing import List, Dict, Tuple, Optional

# ===================== 跨平台配置 =====================
SYSTEM = platform.system()
PY_CMD = "python" if SYSTEM == "Windows" else ("python3" if subprocess.run(["which", "python3"], capture_output=True, text=True).returncode == 0 else "python")
DEL_CMD = "del /q " if SYSTEM == "Windows" else "rm -f "
COMPILE_CMD_CPP = "g++ -std=c++11 {src} -o {exe} -Wall"  # C++ 编译命令

# ===================== 评测结果枚举 =====================
class CaseResult:
    SUCCESS = "SUCCESS"
    COMPILE_ERROR = "COMPILE_ERROR"
    RUN_ERROR = "RUN_ERROR"
    FILE_MISS = "FILE_MISS"
    ANSWER_ERROR = "ANSWER_ERROR"
    SCRIPT_ERROR = "SCRIPT_ERROR"

# ===================== 通用工具函数（已修复） =====================
def read_file_lines(file_path: str) -> List[str]:
    """读取文件所有行，统一处理换行符（移除\r）"""
    if not os.path.exists(file_path):
        return []
    try:
        # 【修复】添加 as f
        with open(file_path, "r", encoding="utf-8", newline="") as f:
            return [line.rstrip("\r") for line in f.readlines()]
    except Exception as e:
        return [f"Read file error: {str(e)}"]

def exec_cmd(cmd: str, timeout: int) -> Tuple[int, str]:
    """执行系统命令，返回退出码+输出信息"""
    try:
        result = subprocess.run(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", timeout=timeout
        )
        return result.returncode, result.stdout
    except subprocess.TimeoutExpired:
        return -2, f"Timeout (>{timeout}s exceeded)"
    except Exception as e:
        return -1, f"Execution failed: {str(e)}"

def compare_files(
    prog_out: str, 
    std_out: str,
    ignore_trailing_spaces: bool = True,
    ignore_leading_spaces: bool = False,
    ignore_empty_lines: bool = False
) -> Tuple[bool, str]:
    """
    【增强】完备的文件比对逻辑
    :param ignore_trailing_spaces: 忽略行尾空格（默认True）
    :param ignore_leading_spaces: 忽略行首空格（默认False）
    :param ignore_empty_lines: 忽略空行比对（默认False）
    """
    prog_lines = read_file_lines(prog_out)
    std_lines = read_file_lines(std_out)
    diff_info = []

    # 预处理行（根据配置忽略空格/空行）
    def preprocess(lines: List[str]) -> List[Tuple[int, str]]:
        processed = []
        for original_idx, line in enumerate(lines):
            stripped = line
            if ignore_trailing_spaces:
                stripped = stripped.rstrip()
            if ignore_leading_spaces:
                stripped = stripped.lstrip()
            if ignore_empty_lines and stripped == "":
                continue
            processed.append((original_idx + 1, stripped))  # (原始行号, 处理后内容)
        return processed

    p_processed = preprocess(prog_lines)
    s_processed = preprocess(std_lines)

    # 行数不一致提示
    if len(p_processed) != len(s_processed):
        diff_info.append(f"Line count mismatch (after preprocessing): Your output ({len(p_processed)}) vs Expected ({len(s_processed)})")

    # 逐行比对
    min_lines = min(len(p_processed), len(s_processed))
    for i in range(min_lines):
        p_original_line, p_content = p_processed[i]
        s_original_line, s_content = s_processed[i]
        if p_content != s_content:
            diff_info.append(
                f"Line mismatch (Your line {p_original_line} vs Expected line {s_original_line}):\n"
                f"  Your output: {prog_lines[p_original_line-1]}"  # 显示原始行
                f"  Expected: {std_lines[s_original_line-1]}"
            )

    # 补充多余/缺失行提示
    if len(p_processed) > len(s_processed):
        diff_info.append(f"\nExtra lines in your output:")
        for i in range(len(s_processed), len(p_processed)):
            p_original_line, _ = p_processed[i]
            diff_info.append(f"  Line {p_original_line}: {prog_lines[p_original_line-1]}")
    elif len(p_processed) < len(s_processed):
        diff_info.append(f"\nMissing lines in your output (expected in standard answer):")
        for i in range(len(p_processed), len(s_processed)):
            s_original_line, _ = s_processed[i]
            diff_info.append(f"  Line {s_original_line}: {std_lines[s_original_line-1]}")

    return len(diff_info) == 0, "\n".join(diff_info)

def auto_match_testcases(case_dir: str) -> Dict[str, Tuple[str, str]]:
    """统一的用例匹配：支持 input1.txt/output1.txt 或 input_abc.in/output_abc.out"""
    all_files = []
    for filename in os.listdir(case_dir):
        file_path = os.path.join(case_dir, filename)
        if os.path.isfile(file_path) and filename.lower().endswith((".txt", ".in", ".out")):
            all_files.append((filename, file_path))

    # 正则匹配：input/output + 标识 + 后缀
    pattern = re.compile(r"^(input|output)(.*?)(\.[a-zA-Z0-9]+)$", re.IGNORECASE)
    input_files: Dict[str, str] = {}
    output_files: Dict[str, str] = {}

    for filename, file_path in all_files:
        match = pattern.match(filename)
        if match:
            prefix = match.group(1).lower()
            identifier = match.group(2).strip()
            std_identifier = identifier.lower().replace(" ", "_")
            if std_identifier == "":
                std_identifier = "default"  # 处理 input.txt/output.txt
            if prefix == "input":
                input_files[std_identifier] = file_path
            elif prefix == "output":
                output_files[std_identifier] = file_path

    # 匹配input/output对
    testcases = {}
    for identifier in input_files.keys():
        if identifier in output_files:
            testcases[identifier] = (input_files[identifier], output_files[identifier])
    return testcases

# ===================== 语言专属评测逻辑 =====================
def compile_cpp(src_file: str, exe_file: str, timeout: int) -> Tuple[CaseResult, str]:
    """C++ 编译逻辑"""
    if not os.path.exists(src_file):
        return CaseResult.FILE_MISS, f"Source file not found: {src_file}"
    
    compile_cmd = COMPILE_CMD_CPP.format(src=src_file, exe=exe_file)
    ret_code, compile_msg = exec_cmd(compile_cmd, timeout)
    if ret_code != 0:
        return CaseResult.COMPILE_ERROR, f"Compile failed:\n{compile_msg}"
    if not os.path.exists(exe_file):
        return CaseResult.RUN_ERROR, "Compile success but no executable generated"
    return CaseResult.SUCCESS, "Compile success!"

def run_cpp_testcase(identifier: str, input_path: str, output_path: str, cfg: Dict) -> Tuple[CaseResult, str]:
    """运行C++测试用例"""
    exe_file = os.path.join(cfg["testcase_dir"], f"judge_temp_{cfg['lang']}{'.exe' if SYSTEM == 'Windows' else '.out'}")
    temp_out = os.path.join(cfg["testcase_dir"], f"temp_{identifier}.txt")
    err_out = os.path.join(cfg["testcase_dir"], f"error_{identifier}.txt")

    # 编译
    compile_res, compile_msg = compile_cpp(cfg["src_file"], exe_file, cfg["timeout"])
    if compile_res != CaseResult.SUCCESS:
        return compile_res, compile_msg

    # 运行命令
    if SYSTEM == "Windows":
        run_cmd = f'"{exe_file}" < "{input_path}" > "{temp_out}"'
    else:
        run_cmd = f'./{os.path.basename(exe_file)} < "{input_path}" > "{temp_out}"'

    # 执行
    ret_code, run_msg = exec_cmd(run_cmd, cfg["timeout"])
    if ret_code != 0:
        if os.path.exists(temp_out):
            exec_cmd(f"{DEL_CMD} {temp_out}", cfg["timeout"])
        if ret_code == -2:
            return CaseResult.RUN_ERROR, f"Timeout (>{cfg['timeout']}s)"
        else:
            return CaseResult.RUN_ERROR, f"Run error (Exit code: {ret_code}):\n{run_msg}"

    # 比对（使用增强后的compare_files）
    if not os.path.exists(temp_out):
        return CaseResult.RUN_ERROR, "No output generated"
    is_correct, diff_info = compare_files(temp_out, output_path)
    if not is_correct:
        mv_cmd = f'rename "{temp_out}" "{os.path.basename(err_out)}"' if SYSTEM == "Windows" else f'mv "{temp_out}" "{err_out}"'
        exec_cmd(mv_cmd, cfg["timeout"])
        return CaseResult.ANSWER_ERROR, f"Answer error:\n{diff_info}\nError output saved to: {err_out}"

    # 清理
    exec_cmd(f"{DEL_CMD} {temp_out}", cfg["timeout"])
    exec_cmd(f"{DEL_CMD} {exe_file}", cfg["timeout"])
    return CaseResult.SUCCESS, "Answer correct!"

def run_python_testcase(identifier: str, input_path: str, output_path: str, cfg: Dict) -> Tuple[CaseResult, str]:
    """运行Python测试用例"""
    temp_out = os.path.join(cfg["testcase_dir"], f"temp_{identifier}.txt")
    err_out = os.path.join(cfg["testcase_dir"], f"error_{identifier}.txt")

    if not os.path.exists(cfg["src_file"]):
        return CaseResult.FILE_MISS, f"Script file not found: {cfg['src_file']}"

    # 运行命令
    run_cmd = f'{PY_CMD} "{cfg["src_file"]}" < "{input_path}" > "{temp_out}"'

    # 执行
    ret_code, run_msg = exec_cmd(run_cmd, cfg["timeout"])
    if ret_code != 0:
        if os.path.exists(temp_out):
            exec_cmd(f"{DEL_CMD} {temp_out}", cfg["timeout"])
        if ret_code == -2:
            return CaseResult.RUN_ERROR, f"Timeout (>{cfg['timeout']}s)"
        elif ret_code == -1:
            return CaseResult.RUN_ERROR, f"Execution failed: {run_msg}"
        else:
            return CaseResult.SCRIPT_ERROR, f"Script error (Exit code: {ret_code}):\n{run_msg}"

    # 比对
    if not os.path.exists(temp_out):
        return CaseResult.RUN_ERROR, "No output generated"
    is_correct, diff_info = compare_files(temp_out, output_path)
    if not is_correct:
        mv_cmd = f'rename "{temp_out}" "{os.path.basename(err_out)}"' if SYSTEM == "Windows" else f'mv "{temp_out}" "{err_out}"'
        exec_cmd(mv_cmd, cfg["timeout"])
        return CaseResult.ANSWER_ERROR, f"Answer error:\n{diff_info}\nError output saved to: {err_out}"

    # 清理
    exec_cmd(f"{DEL_CMD} {temp_out}", cfg["timeout"])
    return CaseResult.SUCCESS, "Answer correct!"

# ===================== 主流程 =====================
def main():
    parser = argparse.ArgumentParser(
        description="Unified Cross-Language Judge (C++/Python)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # 评测C++代码
  python unified_judge.py -l cpp -s main.cpp -t ./testcases -to 10
  # 评测Python脚本
  python unified_judge.py -l python -s main.py -t ./testcases -clean
        """
    )
    parser.add_argument("-l", "--lang", required=True, choices=["cpp", "python"], help="Target language (cpp/python)")
    parser.add_argument("-s", "--src-file", default="main.cpp", help="Source/script file path")
    parser.add_argument("-t", "--testcase-dir", default=".", help="Testcase directory (default: current)")
    parser.add_argument("-to", "--timeout", type=int, default=30, help="Timeout in seconds (default: 30)")
    parser.add_argument("-clean", "--clean-temp", action="store_true", help="Clean temp/error files after judge")
    args = parser.parse_args()

    # 初始化配置
    cfg = {
        "lang": args.lang,
        "src_file": args.src_file,
        "testcase_dir": args.testcase_dir,
        "timeout": args.timeout,
        "clean_temp": args.clean_temp
    }

    # 检查测试用例目录
    if not os.path.isdir(cfg["testcase_dir"]):
        print(f"[Fatal Error] Testcase directory not found: {cfg['testcase_dir']}", file=sys.stderr)
        sys.exit(1)

    # 匹配测试用例
    print(f"\n[Step 1] Auto scanning testcase directory: {cfg['testcase_dir']}")
    testcases = auto_match_testcases(cfg["testcase_dir"])
    if not testcases:
        print("No valid testcase pairs found! (Need inputXXX.txt + outputXXX.txt)", file=sys.stderr)
        sys.exit(0)
    print(f"Found {len(testcases)} valid testcase pairs:")
    for idx, (identifier, (input_path, output_path)) in enumerate(testcases.items(), 1):
        print(f"  {idx}. {os.path.basename(input_path)} ↔ {os.path.basename(output_path)}")

    # 运行测试用例
    print("\n[Step 2] Running testcases...")
    success_count = 0
    fail_count = 0
    fail_details: Dict[str, Tuple[CaseResult, str]] = {}

    run_testcase = run_cpp_testcase if cfg["lang"] == "cpp" else run_python_testcase

    for identifier, (input_path, output_path) in testcases.items():
        print(f"\n---------------------- Testcase: {identifier} ----------------------")
        res, msg = run_testcase(identifier, input_path, output_path, cfg)
        if res == CaseResult.SUCCESS:
            print(f"✅ {identifier}: {msg}")
            success_count += 1
        else:
            print(f"❌ {identifier}: {msg}", file=sys.stderr)
            fail_count += 1
            fail_details[identifier] = (res, msg)

    # 输出报告
    print("\n==================== Judge Report ====================")
    print(f"Total testcases: {len(testcases)}")
    print(f"✅ Passed: {success_count}")
    print(f"❌ Failed: {fail_count}")
    
    if fail_count > 0:
        print("\nFailed testcases details:")
        for identifier, (res, msg) in fail_details.items():
            print(f"\n[{res}] {identifier}:")
            print(f"  {msg}")
        print(f"\nError output files: error_<identifier>.txt (in {cfg['testcase_dir']})")
    else:
        print("\n🎉 All testcases passed!")
    print("======================================================")

    # 清理
    if cfg["clean_temp"]:
        print("\n[Step 3] Cleaning temp/error files...")
        for filename in os.listdir(cfg["testcase_dir"]):
            if filename.startswith(("temp_", "error_", "judge_temp_")):
                file_path = os.path.join(cfg["testcase_dir"], filename)
                exec_cmd(f"{DEL_CMD} {file_path}", cfg["timeout"])
        print("Clean completed!")

    sys.exit(1 if fail_count > 0 else 0)

if __name__ == "__main__":
    main()
