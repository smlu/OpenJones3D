#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

"""Self-test for the Babylon boundary and graph checkers.

Runs check_boundary.py and check_graph.py over the small fake Babylon roots
in tools/testdata/ and proves every check fails on a planted violation and
passes on a clean tree. Run with: python3 test_checks.py
"""

import os
import subprocess
import sys
import unittest

TOOLS = os.path.dirname(os.path.abspath(__file__))
TESTDATA = os.path.join(TOOLS, "testdata")
BOUNDARY = os.path.join(TOOLS, "check_boundary.py")
GRAPH = os.path.join(TOOLS, "check_graph.py")


def run(script, args):
    result = subprocess.run([sys.executable, script] + list(args),
        capture_output=True, text=True)
    return result.returncode, result.stdout + result.stderr


class BoundaryTest(unittest.TestCase):

    def boundary(self, fixture, *args):
        return run(BOUNDARY, list(args) + [os.path.join(TESTDATA, fixture)])

    def test_clean_tree_passes_every_check(self):
        code, out = self.boundary("clean")
        self.assertEqual(code, 0, out)
        self.assertEqual(out, "")

    def test_quoted_include_outside_the_root_fails(self):
        code, out = self.boundary("outside_include", "--only", "includes")
        self.assertEqual(code, 1, out)
        self.assertIn('src/Demo.cpp:1: include not allowed here: "../../../Libs/std/stdFile.h"', out)

    def test_platform_include_in_public_header_fails_includes(self):
        code, out = self.boundary("platform_in_public", "--only", "includes")
        self.assertEqual(code, 1, out)
        self.assertIn("include/babylon/demo/Demo.h:3: include not allowed here: <SDL3/SDL.h>", out)

    def test_platform_include_in_public_header_fails_public(self):
        code, out = self.boundary("platform_in_public", "--only", "public")
        self.assertEqual(code, 1, out)
        self.assertIn("include/babylon/demo/Demo.h:3: public header includes <SDL3/SDL.h>", out)

    def test_platform_include_in_its_backend_passes(self):
        code, out = self.boundary("sdl_in_backend", "--only", "includes")
        self.assertEqual(code, 0, out)

    def test_doctest_outside_tests_fails(self):
        code, out = self.boundary("doctest_outside_tests", "--only", "includes")
        self.assertEqual(code, 1, out)
        self.assertIn("src/Demo.cpp:1: include not allowed here: <doctest.h>", out)

    def test_unknown_header_fails(self):
        code, out = self.boundary("unknown_header", "--only", "includes")
        self.assertEqual(code, 1, out)
        self.assertIn("src/Demo.cpp:1: include not allowed here: <windows.h>", out)

    def test_game_name_as_identifier_prefix_fails(self):
        code, out = self.boundary("game_name", "--only", "names")
        self.assertEqual(code, 1, out)
        self.assertIn("src/Demo.cpp:1: game name in Babylon: sith", out)

    def test_licenses_table_passes(self):
        code, out = self.boundary("licenses_ok", "--only", "licenses")
        self.assertEqual(code, 0, out)

    def test_copyleft_license_row_fails(self):
        code, out = self.boundary("licenses_gpl", "--only", "licenses")
        self.assertEqual(code, 1, out)
        self.assertIn("third_party/README.md:5: non-permissive license for doctest: GPL-3.0", out)

    def test_unlisted_third_party_directory_fails(self):
        code, out = self.boundary("licenses_unlisted_dir", "--only", "licenses")
        self.assertEqual(code, 1, out)
        self.assertIn("third_party/foo:0: third-party directory not listed in third_party/README.md: foo", out)

    def test_fetch_content_without_table_row_fails(self):
        code, out = self.boundary("licenses_unlisted_fetch", "--only", "licenses")
        self.assertEqual(code, 1, out)
        self.assertIn("CMakeLists.txt:1: fetched dependency not listed in third_party/README.md: SDL3", out)

    def test_missing_root_is_a_usage_error(self):
        code, _ = run(BOUNDARY, [os.path.join(TESTDATA, "no_such_fixture")])
        self.assertEqual(code, 2)


class GraphTest(unittest.TestCase):

    def graph(self, dot, allowed):
        return run(GRAPH, [os.path.join(TESTDATA, dot), os.path.join(TESTDATA, allowed)])

    def test_allowed_graph_passes(self):
        code, out = self.graph("graph_ok.dot", "allowed_ok.txt")
        self.assertEqual(code, 0, out)

    def test_extra_edge_fails(self):
        code, out = self.graph("graph_extra_edge.dot", "allowed_ok.txt")
        self.assertEqual(code, 1, out)
        self.assertIn("graph_extra_edge.dot", out)
        self.assertIn("extra dependency edge: core -> SDL3::SDL3", out)

    def test_cycle_fails(self):
        code, out = self.graph("graph_cycle.dot", "allowed_ok.txt")
        self.assertEqual(code, 1, out)
        self.assertIn("graph_cycle.dot", out)
        self.assertIn("dependency cycle among modules: core -> demo -> core", out)

    def test_missing_dot_file_is_a_usage_error(self):
        code, _ = run(GRAPH, [os.path.join(TESTDATA, "no_such_file.dot"),
            os.path.join(TESTDATA, "allowed_ok.txt")])
        self.assertEqual(code, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
