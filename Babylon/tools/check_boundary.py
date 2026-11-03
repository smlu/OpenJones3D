#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

"""Check the Babylon boundary (ADR-0006, ADR-0008, ADR-0015).

Four checks run over every file under the Babylon root, except third_party/,
tools/testdata/ and build directories (any directory containing
CMakeCache.txt):

  includes  every #include in a code file resolves to a Babylon header, a
            C++20 standard header, <doctest.h> in a test location, or a
            third-party header at a location the table below allows
  public    files under */include/babylon/ include only <babylon/...> and
            standard headers
  names     no game name in code or CMake files
  licenses  third_party/README.md lists every vendored directory and every
            FetchContent dependency with a permissive license

Usage: check_boundary.py <babylon-root>
       [--only includes|public|names|licenses]
Exit: 0 clean; 1 with one "<path>:<line>: <message>" line per violation on
stdout, the path relative to the Babylon root; 2 on bad usage.
"""

import argparse
import os
import re
import sys

CODE_SUFFIXES = (".h", ".hpp", ".cpp", ".in")

# Third-party headers and the directory chain they may be included under
# (posix path parts inside the Babylon root). Later modules add rows here
# when their backends arrive (ADR-0009, ADR-0010).
THIRD_PARTY_INCLUDES = (
    ("SDL3/", ("backends", "sdl")),
    ("miniaudio.h", ("backends", "miniaudio")),
)

# Game names that must never appear in Babylon code or CMake files, matched
# as whole words or identifier prefixes, case-sensitive as listed.
GAME_NAMES = ("Jones3D", "Jones", "jones", "Sith", "sith", "Indy",
    "rdroid", "J3D", "j3d", "smush")
GAME_NAME_RE = re.compile(r"(?<![A-Za-z0-9_])(" + "|".join(GAME_NAMES) + r")")

INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*(?:<([^<>]*)>|\"([^\"]*)\")')

# Licenses that may appear in the third_party/README.md table (ADR-0008).
# Compared case-insensitively; a bare "BSD" is not specific enough.
PERMISSIVE_LICENSES = frozenset({
    "mit", "zlib", "bsd-2-clause", "bsd-3-clause", "apache-2.0", "public domain"})

FETCH_DECLARE_RE = re.compile(r"FetchContent_Declare[ \t]*\([ \t]*(\w+)")

# C++20 standard library headers (including the C compatibility headers).
CXX_HEADERS = frozenset("""
    algorithm any array atomic barrier bit bitset cassert cctype cerrno cfenv
    cfloat charconv chrono cinttypes climits clocale cmath codecvt compare
    concurrency complex concepts coroutine csetjmp csignal cstdarg cstddef
    cstdint cstdio cstdlib cstring ctime cuchar cwchar cwctype deque exception
    execution filesystem format forward_list functional future
    initializer_list iomanip ios iosfwd iostream iterator latch limits list
    map memory memory_resource mutex new numeric numbers optional queue
    random ranges ratio regex scoped_allocator semaphore set shared_mutex
    source_location span sstream stdexcept stop_token streambuf string
    string_view syncstream system_error thread tuple type_traits typeinfo
    unordered_map unordered_set utility valarray variant vector version
""".split())


def iter_files(root):
    """Yield posix-relative paths of every checked file under root."""
    for dir_path, dir_names, file_names in os.walk(root):
        if "CMakeCache.txt" in file_names:
            dir_names.clear()
            continue
        dir_names[:] = sorted(d for d in dir_names if d != "third_party")
        rel_dir = os.path.relpath(dir_path, root).replace(os.sep, "/")
        if rel_dir == ".":
            rel_dir = ""
        if rel_dir == "tools/testdata" or rel_dir.startswith("tools/testdata/"):
            dir_names.clear()
            continue
        for name in sorted(file_names):
            yield f"{rel_dir}/{name}" if rel_dir else name


def under_dir(rel, chain):
    """True when the file lives under a directory chain like backends/sdl."""
    dirs = rel.split("/")[:-1]
    size = len(chain)
    return any(dirs[i:i + size] == list(chain) for i in range(len(dirs) - size + 1))


def babylon_header_exists(root, name):
    """True when <babylon/<module>/...> resolves inside the root.

    A generated header counts as present when the matching <name>.in sits
    next to it, e.g. core's Version.h is generated from Version.h.in.
    """
    parts = name.split("/")
    if len(parts) < 3 or not parts[1] or not parts[2]:
        return False
    module = parts[1]
    target = os.path.join(root, module, "include", "babylon", module, *parts[2:])
    return os.path.isfile(target) or os.path.isfile(target + ".in")


def angle_include_allowed(root, rel, name):
    if name.startswith("babylon/"):
        return babylon_header_exists(root, name)
    if name in CXX_HEADERS:
        return True
    if name == "doctest.h":
        return under_dir(rel, ("tests",)) or under_dir(rel, ("third_party",))
    for prefix, chain in THIRD_PARTY_INCLUDES:
        if name == prefix or (prefix.endswith("/") and name.startswith(prefix)):
            return under_dir(rel, chain)
    return False


def quoted_include_resolves_inside(root, rel, quoted):
    base = os.path.dirname(os.path.join(root, rel))
    resolved = os.path.normpath(os.path.join(base, quoted))
    return resolved.startswith(root + os.sep) and os.path.isfile(resolved)


def check_includes(root, rel, lines):
    """Yield (line, message) for every #include that is not allowed here."""
    for num, line in enumerate(lines, 1):
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        angle, quoted = match.group(1), match.group(2)
        if angle is not None:
            name = angle.strip()
            if not angle_include_allowed(root, rel, name):
                yield num, f"include not allowed here: <{name}>"
        elif not quoted_include_resolves_inside(root, rel, quoted):
            yield num, 'include not allowed here: "%s"' % quoted


def check_public(rel, lines):
    """Yield (line, message) for non-babylon, non-standard includes in a
    public header."""
    for num, line in enumerate(lines, 1):
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        angle = match.group(1)
        if angle is not None:
            name = angle.strip()
            if name.startswith("babylon/") or name in CXX_HEADERS:
                continue
            text = f"<{name}>"
        else:
            text = '"%s"' % match.group(2)
        yield num, f"public header includes {text}"


def check_names(lines):
    """Yield (line, message) for every game name starting an identifier."""
    for num, line in enumerate(lines, 1):
        pos = 0
        while True:
            match = GAME_NAME_RE.search(line, pos)
            if not match:
                break
            yield num, f"game name in Babylon: {match.group(1)}"
            pos = match.end()


def table_rows(lines):
    """Yield (line, cells) for the data rows of a markdown table, skipping
    the header and separator rows."""
    for num, line in enumerate(lines, 1):
        text = line.strip()
        if not text.startswith("|"):
            continue
        cells = [cell.strip() for cell in text.strip("|").split("|")]
        if len(cells) < 5 or cells[0].lower() == "name":
            continue
        if all(cell and set(cell) <= set("-: ") for cell in cells):
            continue
        yield num, cells


def check_licenses(root):
    """Check third_party/README.md (ADR-0008): permissive licenses and one
    row per vendored directory. Returns (violations, lowercased table names)."""
    if not os.path.isdir(os.path.join(root, "third_party")):
        return [], set()
    readme = os.path.join(root, "third_party", "README.md")
    if not os.path.isfile(readme):
        return [("third_party", 0, "third_party/README.md missing")], set()

    with open(readme, encoding="utf-8", errors="replace") as fh:
        lines = fh.read().splitlines()
    violations = []
    names = set()
    files_columns = []
    for num, cells in table_rows(lines):
        name, license_, files = cells[0], cells[2], cells[4]
        names.add(name.lower())
        files_columns.append(files)
        if license_.lower() not in PERMISSIVE_LICENSES:
            violations.append(("third_party/README.md", num,
                f"non-permissive license for {name}: {license_}"))
    for entry in sorted(os.listdir(os.path.join(root, "third_party"))):
        if not os.path.isdir(os.path.join(root, "third_party", entry)):
            continue
        if not any(f"{entry}/" in files for files in files_columns):
            violations.append((f"third_party/{entry}", 0,
                f"third-party directory not listed in third_party/README.md: {entry}"))
    return violations, names


def is_public_header(rel):
    dirs = rel.split("/")[:-1]
    return any(dirs[i:i + 2] == ["include", "babylon"] for i in range(len(dirs) - 1))


def is_cmake_file(rel):
    return os.path.basename(rel) == "CMakeLists.txt" or rel.endswith(".cmake")


def main(argv):
    parser = argparse.ArgumentParser(
        description="Check the Babylon boundary (ADR-0006, ADR-0008, ADR-0015).")
    parser.add_argument("root", metavar="babylon-root",
        help="the Babylon directory to check")
    parser.add_argument("--only", choices=("includes", "public", "names", "licenses"),
        help="run only this check (used by tools/test_checks.py)")
    args = parser.parse_args(argv)

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        parser.error(f"not a directory: {args.root}")

    violations = []
    fetched = []
    for rel in iter_files(root):
        code = rel.endswith(CODE_SUFFIXES)
        cmake = is_cmake_file(rel)
        if not code and not cmake:
            continue
        with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as fh:
            lines = fh.read().splitlines()
        if code and args.only in (None, "includes"):
            violations += [(rel, num, text) for num, text in check_includes(root, rel, lines)]
        if code and is_public_header(rel) and args.only in (None, "public"):
            violations += [(rel, num, text) for num, text in check_public(rel, lines)]
        if args.only in (None, "names"):
            violations += [(rel, num, text) for num, text in check_names(lines)]
        if cmake and args.only in (None, "licenses"):
            for num, line in enumerate(lines, 1):
                match = FETCH_DECLARE_RE.search(line)
                if match:
                    fetched.append((rel, num, match.group(1)))

    if args.only in (None, "licenses"):
        license_violations, table_names = check_licenses(root)
        violations += license_violations
        for rel, num, name in fetched:
            if name.lower() not in table_names:
                violations.append((rel, num,
                    f"fetched dependency not listed in third_party/README.md: {name}"))

    for path, num, message in sorted(violations):
        print(f"{path}:{num}: {message}")
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
