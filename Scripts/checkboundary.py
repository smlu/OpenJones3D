# This script measures how much Windows and DirectX use remains above the platform
# layer, by scanning the source for forbidden system headers and Windows/DirectX
# identifiers. OpenJones3D is moving all Windows and DirectX use behind a platform
# layer, so this shows what is left and stops the amount from growing.
#
# It only scans tracked source files under Jones3D/ and Libs/, so build trees and
# game assets never count. Comments and string literals are blanked first, keeping
# line numbers, so matches inside them are ignored.
#
# Usage:
#   python checkboundary.py
#
# Extra arguments:
#   --list <path>:    Print each match in a file or directory, one per line
#   --check:          Compare with the allowlist, exit 1 on any problem
#   --update:         Rewrite the allowlist from the measured counts
#   --allow-increase: With --update, allow counts to go up and files to be added
#
# Example usage:
#   python checkboundary.py --check
#   python checkboundary.py --list Libs/sith
#   python checkboundary.py --update --allow-increase

import argparse, bisect, os, re, subprocess, sys
from typing import Dict, List, Tuple

# Hardcoded root folder path
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_FOLDER = f"{SCRIPT_DIR}/../"

# Allowlist file with one "<repo path> <allowed count>" per line, maintained by --update
ALLOWLIST_FILE = "Scripts/boundary-allowlist.txt"

# Paths in the platform layer, directories or single files. Everything there may
# use Windows and DirectX freely and is not counted.
PLATFORM_PATHS = [
    "Libs/std/Win95/",
    "Libs/wkernel/",
    "Libs/w32util/",
    "Libs/sound/DriverDX6.c",
    "Libs/sound/DriverDX9.c",
]

# Files exempt from the check because they are the Windows injector and the dialog
# resources.
EXEMPT_FILES = [
    "Jones3D/exemain.cpp",
    "Jones3D/Indy3D.rc",
]

# Forbidden system headers, matched on the base name, case-insensitively.
FORBIDDEN_INCLUDES = [
    "commctrl.h",
    "commdlg.h",
    "d3d.h",
    "d3d9.h",
    "d3dcommon.h",
    "d3dcompiler.h",
    "dbghelp.h",
    "ddraw.h",
    "dinput.h",
    "dplay.h",
    "dplobby.h",
    "dsound.h",
    "mmreg.h",
    "mmsystem.h",
    "objbase.h",
    "shellapi.h",
    "shlobj.h",
    "windows.h",
    "windowsx.h",
    "winsock.h",
    "winsock2.h",
    "xinput.h",
]

# Forbidden Windows/DirectX identifiers, matched as whole words.
FORBIDDEN_WORDS = [
    "APIENTRY",
    "BOOL",
    "BYTE",
    "CALLBACK",
    "DWORD",
    "HBITMAP",
    "HBRUSH",
    "HCURSOR",
    "HDC",
    "HFONT",
    "HANDLE",
    "HICON",
    "HINSTANCE",
    "HKEY",
    "HMODULE",
    "HRESULT",
    "HWND",
    "LONG",
    "LPCSTR",
    "LPSTR",
    "LPVOID",
    "LPARAM",
    "LRESULT",
    "MSG",
    "POINT",
    "RECT",
    "UINT",
    "WINAPI",
    "WORD",
    "WPARAM",
    "GUID",
    "DPID",
]

# Forbidden Windows/DirectX identifier patterns, where '*' means anything. Matched
# as whole words. tSys* counts because those types are backend types, though they
# are declared in shared headers.
FORBIDDEN_PATTERNS = [
    "WM_\\w*",
    "VK_\\w*",
    "DIK_\\w*",
    "DD[A-Z]*_\\w*",
    "DS(?!ND_)[A-Z0-9]*_\\w*",
    "D3D\\w*",
    "LPD3D\\w*",
    "LPDIRECT\\w*",
    "IDirect\\w*",
    "tSys[A-Z]\\w*",
]

# Source file types that are scanned.
SCAN_EXTENSIONS = (".c", ".cpp", ".h", ".rc")

# Every match found in a file: (line number, match text)
Match = Tuple[int, str]

FORBIDDEN_INCLUDE_SET = frozenset(FORBIDDEN_INCLUDES)
IDENTIFIER_PATTERN = re.compile(
    "|".join([rf"\b{re.escape(word)}\b" for word in FORBIDDEN_WORDS]
             + [rf"\b{pattern}\b" for pattern in FORBIDDEN_PATTERNS]))
INCLUDE_PATTERN = re.compile(r"^[ \t]*#[ \t]*include[ \t]*(<[^>\n]+>|\"[^\"\n]+\")", re.MULTILINE)

def is_candidate_path(path: str) -> bool:
    """
    Tell whether a tracked repo path is a source file of the engine at all,
    before the platform layer and exempt file rules are applied.

    Args:
        path (str): The repo relative path with forward slashes.

    Returns:
        bool: True when the path is a scanned source file type under Jones3D/ or Libs/.
    """
    parts: List[str] = path.split("/")
    if len(parts) < 2:
        return False
    if parts[0] not in ("Jones3D", "Libs"):
        return False
    if path.endswith(".h.in"):
        return True
    if not path.endswith(SCAN_EXTENSIONS):
        return False
    if path.startswith("Libs/external/"):
        return False
    for part in parts[:-1]:
        if part in ("RTI", "Tests"):
            return False
    return True

def is_scanned_path(path: str) -> bool:
    """
    Tell whether a tracked engine source file is counted by the boundary check.

    Args:
        path (str): The repo relative path with forward slashes.

    Returns:
        bool: True when the path is a candidate not in the platform layer and not exempt.
    """
    if not is_candidate_path(path):
        return False
    for platform_path in PLATFORM_PATHS:
        if path.startswith(platform_path):
            return False
    if path in EXEMPT_FILES:
        return False
    return True

def get_tracked_files() -> List[str]:
    """
    List tracked repo paths the boundary check counts, from git so build trees
    never count.

    Returns:
        List[str]: Sorted repo relative paths with forward slashes.
    """
    result = subprocess.run(["git", "ls-files"], cwd=ROOT_FOLDER, capture_output=True, check=False)
    if result.returncode != 0:
        sys.stderr.write("Error: this script must run inside the OpenJones3D git repository\n")
        sys.exit(1)
    files: List[str] = []
    for line in result.stdout.decode("utf-8", "replace").splitlines():
        path = line.strip()
        if not path or not is_scanned_path(path):
            continue
        files.append(path)
    return sorted(files)

def mask_source(content: str) -> str:
    """
    Blank out comments, string literals and char literals with spaces, keeping the
    same line count and every newline, so token positions stay valid. Quoted
    #include names are read separately from the raw text before this runs.

    Args:
        content (str): The raw file content.

    Returns:
        str: The masked content, same length as the input.
    """
    chars: List[str] = list(content)
    num_chars: int = len(chars)
    i = 0
    while i < num_chars:
        c: str = chars[i]

        # Comments, including ones running across lines
        if c == "/" and i + 1 < num_chars:
            nxt: str = chars[i + 1]
            if nxt == "/":
                while i < num_chars and chars[i] != "\n":
                    chars[i] = " "
                    i += 1
                continue
            if nxt == "*":
                chars[i] = " "
                chars[i + 1] = " "
                i += 2
                while i < num_chars:
                    if chars[i] == "*" and i + 1 < num_chars and chars[i + 1] == "/":
                        chars[i] = " "
                        chars[i + 1] = " "
                        i += 2
                        break
                    if chars[i] != "\n":
                        chars[i] = " "
                    i += 1
                continue

        # String literals, with escapes; they may not span lines
        if c == '"':
            chars[i] = " "
            i += 1
            while i < num_chars and chars[i] != "\n":
                if chars[i] == "\\":
                    chars[i] = " "
                    if i + 1 < num_chars and chars[i + 1] != "\n":
                        chars[i + 1] = " "
                    i += 2
                    continue
                is_string_end: bool = chars[i] == '"'
                chars[i] = " "
                i += 1
                if is_string_end:
                    break
            continue

        # Char literals, with escapes; they may not span lines
        if c == "'":
            chars[i] = " "
            i += 1
            while i < num_chars and chars[i] != "\n":
                if chars[i] == "\\":
                    chars[i] = " "
                    if i + 1 < num_chars and chars[i + 1] != "\n":
                        chars[i + 1] = " "
                    i += 2
                    continue
                is_char_end: bool = chars[i] == "'"
                chars[i] = " "
                i += 1
                if is_char_end:
                    break
            continue

        i += 1
    return "".join(chars)

def line_starts(content: str) -> List[int]:
    """
    Collect the start offsets of every line, plus one past the end, so a match at
    any offset can be turned into its line number.

    Args:
        content (str): The raw file content.

    Returns:
        List[int]: Start offsets of each line, in order.
    """
    starts: List[int] = [0]
    pos: int = content.find("\n")
    while pos >= 0:
        starts.append(pos + 1)
        pos = content.find("\n", pos + 1)
    return starts

def is_forbidden_include(name: str) -> bool:
    """
    Tell whether an include name matches the forbidden header list, matched
    case-insensitively on the base name with path prefixes kept for readability.

    Args:
        name (str): The include name without the surrounding <, > or quotes.

    Returns:
        bool: True when the include targets a forbidden system header.
    """
    base_name = name.replace("\\", "/").rsplit("/", 1)[-1].lower()
    return base_name in FORBIDDEN_INCLUDE_SET

def scan_file(path: str) -> List[Match]:
    """
    Scan one counted file for forbidden includes and identifiers.

    Args:
        path (str): The repo relative path with forward slashes.

    Returns:
        List[Match]: Every match in the file as (line number, match text), in file order.
    """
    with open(os.path.join(ROOT_FOLDER, path), "r", encoding="latin-1") as file:
        content: str = file.read()

    masked: str = mask_source(content)
    starts: List[int] = line_starts(content)

    def line_of(pos: int) -> int:
        return bisect.bisect_right(starts, pos)

    matches: List[Match] = []

    # Include names are read from the raw text, since masking blanks quoted names.
    # A directive only counts when its # is still in the masked text in place.
    for include_match in INCLUDE_PATTERN.finditer(content):
        hash_pos: int = content.find("#", include_match.start())
        if masked[hash_pos] != "#":
            continue
        name: str = include_match.group(1)[1:-1]
        if not is_forbidden_include(name):
            continue
        directive: str = content[include_match.start():include_match.end()].strip()
        matches.append((line_of(include_match.start()), directive))

    for ident_match in IDENTIFIER_PATTERN.finditer(masked):
        matches.append((line_of(ident_match.start()), ident_match.group(0)))

    return matches

def module_of(path: str) -> str:
    """
    Give the module table name of a counted file: Jones3D, or the first two path
    parts under Libs/, with the platform free part of std named separately.

    Args:
        path (str): The repo relative path with forward slashes.

    Returns:
        str: The module name used in the report.
    """
    parts: List[str] = path.split("/")
    if parts[0] == "Jones3D":
        return "Jones3D"
    module: str = "/".join(parts[:2])
    if module == "Libs/std":
        module = "Libs/std/General"
    return module

def collect_matches(paths: List[str]) -> Dict[str, List[Match]]:
    """
    Scan every counted file.

    Args:
        paths (List[str]): The counted repo paths.

    Returns:
        Dict[str, List[Match]]: Matches per repo path, for every file that has matches.
    """
    counts: Dict[str, List[Match]] = {}
    for path in paths:
        matches: List[Match] = scan_file(path)
        if matches:
            counts[path] = matches
    return counts

def print_report(counts: Dict[str, List[Match]], num_scanned: int) -> None:
    """
    Print the default report: matches per module, the highest files and the total.

    Args:
        counts (Dict[str, List[Match]]): Matches per repo path.
        num_scanned (int): Total number of scanned files.
    """
    modules: Dict[str, int] = {}
    module_files: Dict[str, int] = {}
    for path, matches in counts.items():
        module = module_of(path)
        modules[module] = modules.get(module, 0) + len(matches)
        module_files[module] = module_files.get(module, 0) + 1

    print("Windows/DirectX matches above the platform layer:")
    print("-" * 48)
    for module in sorted(modules):
        print(f"{module + ':':<20}{modules[module]:>8} matches ({module_files[module]:>2} files)")

    print("")
    print("Top files:")
    top_files: List[Tuple[str, int]] = sorted(
        ((path, len(matches)) for path, matches in counts.items()), key=lambda item: (-item[1], item[0]))
    for path, num_matches in top_files[:20]:
        print(f"{num_matches:>12}  {path}")

    num_matches_total: int = sum(len(matches) for matches in counts.values())
    print("")
    print(f"Total: {num_matches_total} matches in "
          f"{len(counts)} of {num_scanned} scanned files")

def read_allowlist() -> Dict[str, int]:
    """
    Read and validate the allowlist file.

    Returns:
        Dict[str, int]: Allowed match count per repo path. Exits when the file has a bad line.
    """
    allowlist_path: str = os.path.join(ROOT_FOLDER, ALLOWLIST_FILE)
    if not os.path.exists(allowlist_path):
        return {}

    entries: Dict[str, int] = {}
    with open(allowlist_path, "r", encoding="utf-8") as file:
        for line_num, line in enumerate(file, 1):
            text: str = line.strip()
            if not text or text.startswith("#"):
                continue
            items: List[str] = text.split()
            if len(items) != 2:
                sys.stderr.write(f"Error: bad allowlist line {line_num}: '{text}'\n")
                sys.exit(1)
            try:
                count: int = int(items[1])
            except ValueError:
                sys.stderr.write(f"Error: bad allowlist count on line {line_num}: '{text}'\n")
                sys.exit(1)
            if count < 0:
                sys.stderr.write(f"Error: negative allowlist count on line {line_num}: '{text}'\n")
                sys.exit(1)
            entries[items[0]] = count
    return entries

def write_allowlist(entries: Dict[str, int]) -> None:
    """
    Write the allowlist file, sorted by path with forward slashes on every host.

    Args:
        entries (Dict[str, int]): Allowed match count per repo path.
    """
    lines: List[str] = [
        "# Boundary allowlist for Scripts/checkboundary.py, one line per file: <path> <allowed matches>.",
        "# Maintained with: python Scripts/checkboundary.py --update, covered by --check.",
        "# Files with no matches need no entry. Sorted by path.",
        "",
    ]
    for path in sorted(entries):
        lines.append(f"{path} {entries[path]}")
    with open(os.path.join(ROOT_FOLDER, ALLOWLIST_FILE), "w", encoding="utf-8", newline="\n") as file:
        file.write("\n".join(lines) + "\n")

def run_list(list_path: str) -> None:
    """
    Print each match in a file or directory, one per line.

    Args:
        list_path (str): The repo relative path to list matches under.
    """
    prefix: str = list_path.strip().replace("\\", "/").rstrip("/")
    files: List[str] = [path for path in get_tracked_files()
                        if path == prefix or path.startswith(f"{prefix}/")]
    if not files:
        sys.stderr.write(f"Error: no scanned files under '{list_path}'\n")
        sys.exit(1)

    counts: Dict[str, List[Match]] = {path: scan_file(path) for path in files}
    match_entries: List[Tuple[str, int, str]] = [
        (path, line, match) for path, matches in counts.items() for line, match in matches
    ]
    for path, line, match in sorted(match_entries):
        print(f"{path}:{line}: {match}")

def run_check() -> None:
    """
    Compare measured counts with the allowlist and exit 1 on any problem.
    """
    paths: List[str] = get_tracked_files()
    counts: Dict[str, List[Match]] = collect_matches(paths)
    allowlist: Dict[str, int] = read_allowlist()

    problems: List[str] = []
    needs_allow_increase: bool = False

    for path in sorted(counts):
        num_matches: int = len(counts[path])
        if path not in allowlist:
            problems.append(f"{path}: {num_matches} matches but no allowlist entry")
            needs_allow_increase = True
        elif num_matches > allowlist[path]:
            problems.append(f"{path}: {num_matches} matches, entry allows {allowlist[path]}")
            needs_allow_increase = True

    for path, entry in sorted(allowlist.items()):
        if not is_scanned_path(path):
            problems.append(f"{path}: entry is {entry} but the file is no longer tracked or scanned")
            continue
        num_matches = len(counts.get(path, []))
        if entry > num_matches:
            problems.append(f"{path}: entry is {entry} but the file has {num_matches} matches")

    if problems:
        print(f"Boundary check failed, {len(problems)} problem(s):")
        for problem in problems:
            print(f"    {problem}")
        if needs_allow_increase:
            print("Run: python Scripts/checkboundary.py --update --allow-increase to accept the growth,")
            print("or: python Scripts/checkboundary.py --update to clean up stale entries first.")
        else:
            print("Run: python Scripts/checkboundary.py --update to refresh the allowlist.")
        sys.exit(1)

    print(f"Boundary check OK: {len(counts)} files with matches, {sum(len(m) for m in counts.values())} matches, "
          f"all within the allowlist.")

def run_update(allow_increase: bool) -> None:
    """
    Rewrite the allowlist from the measured counts, refusing to make any count go
    up or add a file unless the increase is allowed. Stale entries are dropped.

    Args:
        allow_increase (bool): True to accept raised counts and added files.
    """
    paths: List[str] = get_tracked_files()
    counts: Dict[str, List[Match]] = collect_matches(paths)
    allowlist: Dict[str, int] = read_allowlist()

    real_counts: Dict[str, int] = {path: len(matches) for path, matches in counts.items()}

    refusals: List[str] = []
    for path in sorted(real_counts):
        num_matches: int = real_counts[path]
        if num_matches > allowlist.get(path, 0):
            if path in allowlist:
                refusals.append(f"{path}: {num_matches} matches, entry allows {allowlist[path]}")
            else:
                refusals.append(f"{path}: {num_matches} matches, no entry")
    if not allow_increase and refusals:
        print("Refusing to update the allowlist, counts would go up or files would be added:")
        for refusal in refusals:
            print(f"    {refusal}")
        print("If the growth is approved, run: python Scripts/checkboundary.py --update --allow-increase")
        sys.exit(1)

    # Every counted file with matches gets an entry at its real count; entries for
    # files with no matches or that are no longer scanned are dropped, as the
    # header says files with no matches need no entry.
    new_entries: Dict[str, int] = dict(real_counts)

    write_allowlist(new_entries)
    print(f"Wrote {ALLOWLIST_FILE} with {len(new_entries)} entries.")

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Measure Windows/DirectX use remaining above the platform layer.")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--list", metavar="path", help="print each match in a file or directory")
    modes.add_argument("--check", action="store_true", help="compare with the allowlist, exit 1 on problems")
    modes.add_argument("--update", action="store_true", help="rewrite the allowlist from the measured counts")
    parser.add_argument("--allow-increase", action="store_true",
                        help="with --update, allow counts to go up and files to be added")
    args = parser.parse_args()

    if args.allow_increase and not args.update:
        parser.error("--allow-increase is only used with --update")

    if args.list:
        run_list(args.list)
    elif args.check:
        run_check()
    elif args.update:
        run_update(args.allow_increase)
    else:
        paths: List[str] = get_tracked_files()
        counts: Dict[str, List[Match]] = collect_matches(paths)
        print_report(counts, len(paths))

if __name__ == "__main__":
    main()
