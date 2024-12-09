# This script analyzes the progress of implementation by going over all modules source files
# and searching for the `{module_name}_InstallHooks` function. In this function, it counts
# how many files' functions are hooked to the address in the game's binary.
#
# Usage:
#   python analyze.py
#
# Extra arguments:
#   --detail: Print full detailed progress statistics
#   --module: Print progress statistics for specific module(s). Can be specified multiple times.
#
# Example usage:
#   python analyze.py --detail
#   python analyze.py --module module1 --module module2 --module module3

import argparse, os, re
from typing import Tuple, List, Dict, Optional

# Hardcoded root folder path
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_FOLDER = f"{SCRIPT_DIR}/../"

# Modules dictionary with module names as keys and relative paths as values
MODULES = {
    "rdroid": "Libs/rdroid",
    "sith": "Libs/sith",
    "sound": "Libs/sound",
    "std": "Libs/std",
    "w32util": "Libs/w32util",
    "wkernel": "Libs/wkernel",
    "Jones3D": "Jones3D"
}

FileProgress = Tuple[str, int, int]
FolderProgress = Tuple[Dict[str, Tuple[int, int]], int, int]

def scan_file(filepath: str) -> Optional[FileProgress]:
    """
    Scan a single .c file for *_InstallHooks(void) functions and count J3D_HOOKFUNC calls.

    Args:
        filepath (str): The path to the C file.

    Returns:
        Optional[FileProgress]: A tuple containing the base file name, the number of implemented functions,
                                        and the total number of functions. Returns None if no InstallHooks function is found.
    """
    with open(filepath, 'r', encoding='utf-8') as file:
        content: str = file.read()

    # Extract the base name of the file without extension
    base_name: str = os.path.basename(filepath).split('.')[0]

    # Regex to find the InstallHooks function
    install_hooks_pattern: re.Pattern = re.compile(rf'{base_name}_InstallHooks\s*\(\s*void\s*\)\s*\{{.*?^\}}', re.DOTALL | re.MULTILINE)
    install_hooks_functions: list[str] = install_hooks_pattern.findall(content)

    if not install_hooks_functions:
        return None

    total_funcs: int = 0
    implemented_funcs: int = 0

    # Regex to find uncommented and commented J3D_HOOKFUNC calls
    j3d_hookfunc_pattern: re.Pattern = re.compile(r'J3D_HOOKFUNC\s*\(', re.MULTILINE)
    comment_pattern: re.Pattern = re.compile(r'^\s*//.*J3D_HOOKFUNC\s*\(', re.MULTILINE)

    for func in install_hooks_functions:
        total_calls: int = len(j3d_hookfunc_pattern.findall(func))
        commented_calls: int = len(comment_pattern.findall(func))
        uncommented_calls: int = total_calls - commented_calls

        total_funcs += total_calls
        implemented_funcs += uncommented_calls

    return base_name, implemented_funcs, total_funcs

def scan_folder(folder: str) -> FolderProgress:
    """
    Recursively scan a folder for .c files and collect statistics.

    Args:
        folder (str): The path to the folder containing the source files.

    Returns:
        FolderProgress: A dictionary with file progress (implemented, total), total implemented, and total functions.
    """
    progress: Dict[str, Tuple[int, int]] = {}
    total_funcs: int = 0
    total_implemented: int = 0

    for root, _, files in os.walk(folder):
        for file in files:
            if file.endswith('.c'):
                filepath: str = os.path.join(root, file)
                result = scan_file(filepath)

                if result:
                    file_name, implemented, total = result
                    progress[file_name] = (implemented, total)
                    total_funcs += total
                    total_implemented += implemented

    return progress, total_implemented, total_funcs

def print_module_stats(module_name: str, implemented: int, total: int, align: bool = True) -> None:
    percentage = (implemented / total) * 100 if total > 0 else 0
    if align:
        print(f"{module_name + ':':<9}{percentage:>6.2f}% ({implemented}/{total})")
    else:
        print(f"{module_name + ':'} {percentage:.2f}% ({implemented}/{total})")

def print_detailed_module_stats(module_name: str, progress: Dict[str, Tuple[int, int]], implemented: int, total: int) -> None:

    print_module_stats(module_name, implemented, total, align=False)

    # Print per file progress
    for file_name, (implemented, total) in progress.items():
        if (total == 0):
            continue
        percentage: float = (implemented / total * 100) if total > 0 else 0
        if module_name == 'sith':
            print(f"    {file_name + ':':<23}{percentage:>6.2f}% ({implemented}/{total})")
        else:
            print(f"    {file_name + ':':<15}{percentage:>6.2f}% ({implemented}/{total})")


    print("") # add extra new line

def print_total_stats(total_implemented: int, total_functions: int) -> None:
    total_progress = (total_implemented / total_functions) * 100 if total_functions > 0 else 0
    print(f"\nOverall Progress: {total_progress:.2f}% | {total_implemented} implemented out of {total_functions:}\n")


def analyze_progress(modules: List[str], detailed: bool):
    print("Module Progress:")
    print("-" * 40)

    total_implemented = 0
    total_functions = 0
    for module_name in modules:
        # Build the full path to the module
        module_path = os.path.join(ROOT_FOLDER, MODULES[module_name])

        # Calculate the progress for the module
        module_file_progress, implemented, total = scan_folder(module_path)

        # Print the individual module stats
        if detailed:
            print_detailed_module_stats(module_name, module_file_progress, implemented, total)
        else:
            print_module_stats(module_name, implemented, total)

        # Add to the overall total
        total_implemented += implemented
        total_functions += total

    # Print overall progress
    print_total_stats(total_implemented, total_functions)

def list_of_strings(arg: str):
    return arg.strip().split(',')

def main() -> None:
    parser = argparse.ArgumentParser(description="Analyze implementation progress by scanning source files.")
    parser.add_argument('-d', '--detail', action='store_true', help='print full detailed progress statistics')
    parser.add_argument('-m', '--module', action='append', type=list_of_strings,  help='print progress statistics for specific module(s)')
    args = parser.parse_args()

    modules: List[str] = list(MODULES.keys())
    if args.module:
        modules = []
        for mlist in args.module:
            for m in mlist:
                m = m.strip()
                if m not in MODULES:
                    print(f"Error: Invalid module name '{m}'!")
                    return
                modules.append(m)

    analyze_progress(modules, args.detail)

if __name__ == "__main__":
    main()