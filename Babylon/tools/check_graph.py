#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

"""Check the Babylon module dependency graph against allowed-deps.txt (ADR-0015).

The dot file comes from a Babylon-alone configure with
`cmake --graphviz=<file>`; CMake writes one .dot file per target next to the
main one, only the main file given here is read. For every Babylon module
target babylon_<module> (not babylon_<module>_tests, not babylon_doctest*)
the set of directly linked targets must equal the set in allowed-deps.txt,
and the modules must form an acyclic graph.

Usage: check_graph.py <graph.dot> <allowed-deps.txt>
Exit: 0 clean; 1 with one "<path>:<line>: <message>" line per violation on
stdout, paths as given; 2 on bad usage.
"""

import argparse
import os
import re
import sys

NODE_RE = re.compile(r'^[ \t]*"(node\d+)"[ \t]*\[[ \t]*label[ \t]*='
    r'[ \t]*"((?:[^"\\]|\\.)*)"')
EDGE_RE = re.compile(r'^[ \t]*"(node\d+)"[ \t]*->[ \t]*"(node\d+)"')

MODULE_PREFIX = "babylon_"


def module_of(target):
    """Module name for a babylon_<module> target, None for any other target."""
    if not target.startswith(MODULE_PREFIX):
        return None
    rest = target[len(MODULE_PREFIX):]
    if not rest or rest.endswith("_tests") or rest.startswith("doctest"):
        return None
    return rest


def dep_name(target):
    """Map babylon_<x> to <x>; keep other names, e.g. SDL3::SDL3, as they are."""
    return target[len(MODULE_PREFIX):] if target.startswith(MODULE_PREFIX) else target


def parse_graph(path):
    """Return (target -> node line, edges as (src target, dst target, line))."""
    node_target = {}
    node_line = {}
    edges = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        for num, line in enumerate(fh, 1):
            match = NODE_RE.match(line)
            if match:
                # Library labels read "<target>\n(<alias>)"; keep the target.
                target = match.group(2).split("\\n", 1)[0].strip()
                if match.group(1) not in node_target:
                    node_target[match.group(1)] = target
                    node_line[target] = num
                continue
            match = EDGE_RE.match(line)
            if match:
                edges.append((match.group(1), match.group(2), num))
    return node_target, node_line, edges


def parse_allowed(path):
    """Return module -> (line, deps set); raise ValueError on a bad line."""
    allowed = {}
    with open(path, encoding="utf-8", errors="replace") as fh:
        for num, line in enumerate(fh, 1):
            text = line.split("#", 1)[0].strip()
            if not text:
                continue
            module, sep, rest = text.partition(":")
            module = module.strip()
            if not sep or not module or not module.isidentifier():
                raise ValueError(f"{path}:{num}: expected '<module>: <deps>', got: {text}")
            allowed[module] = (num, frozenset(rest.split()))
    return allowed


def find_cycle(modules, edges_between):
    """Return one cycle as [a, b, a], or None."""
    color = dict.fromkeys(modules, 0)
    stack = []

    def visit(node):
        color[node] = 1
        stack.append(node)
        for nxt in sorted(edges_between[node]):
            if color[nxt] == 1:
                return stack[stack.index(nxt):] + [nxt]
            if color[nxt] == 0:
                found = visit(nxt)
                if found:
                    return found
        stack.pop()
        color[node] = 2
        return None

    for module in sorted(modules):
        if color[module] == 0:
            found = visit(module)
            if found:
                return found
    return None


def main(argv):
    parser = argparse.ArgumentParser(
        description="Check the Babylon module dependency graph (ADR-0015).")
    parser.add_argument("graph", metavar="graph.dot",
        help="the main dot file of a cmake --graphviz run")
    parser.add_argument("allowed", metavar="allowed-deps.txt",
        help="the module dependency allowlist")
    args = parser.parse_args(argv)

    for path in (args.graph, args.allowed):
        if not os.path.isfile(path):
            parser.error(f"not a file: {path}")

    node_target, node_line, raw_edges = parse_graph(args.graph)
    try:
        allowed = parse_allowed(args.allowed)
    except ValueError as error:
        print(f"check_graph.py: error: {error}", file=sys.stderr)
        return 2

    modules = sorted({module for target in node_target.values()
        if (module := module_of(target)) is not None})
    module_set = set(modules)

    # Direct links of every module: dependency name -> first edge line.
    deps = {module: {} for module in modules}
    module_edges = {module: set() for module in modules}
    for src_id, dst_id, num in raw_edges:
        module = module_of(node_target.get(src_id, ""))
        if module is None or dst_id not in node_target:
            continue
        dep = dep_name(node_target[dst_id])
        deps[module].setdefault(dep, num)
        if dep in module_set:
            module_edges[module].add(dep)

    violations = []
    for module in modules:
        if module not in allowed:
            violations.append((args.graph, node_line["babylon_" + module],
                f"module not in allowed-deps.txt: {module}"))
            continue
        declared_line, declared_deps = allowed[module]
        for dep in sorted(set(deps[module]) - declared_deps):
            violations.append((args.graph, deps[module][dep],
                f"extra dependency edge: {module} -> {dep}"))
        for dep in sorted(declared_deps - set(deps[module])):
            violations.append((args.allowed, declared_line,
                f"missing dependency edge: {module} -> {dep}"))
    for module in sorted(set(allowed) - set(modules)):
        violations.append((args.allowed, allowed[module][0],
            f"module in allowed-deps.txt but not in the graph: {module}"))

    cycle = find_cycle(modules, module_edges)
    if cycle:
        violations.append((args.graph, deps[cycle[0]][cycle[1]],
            "dependency cycle among modules: " + " -> ".join(cycle)))

    for path, num, message in sorted(violations):
        print(f"{path}:{num}: {message}")
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
