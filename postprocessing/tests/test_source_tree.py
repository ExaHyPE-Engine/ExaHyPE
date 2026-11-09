# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import contextlib
import io
import json
import os
import stat
import sys
import tempfile
import unittest

from unittest import mock

from postprocessing.source_tree import Finding
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import clang_findings
from postprocessing.source_tree import compilation_arguments
from postprocessing.source_tree import find_repository_root
from postprocessing.source_tree import is_excluded_directory
from postprocessing.source_tree import is_source_path
from postprocessing.source_tree import major_version
from postprocessing.source_tree import positive_integer
from postprocessing.source_tree import report
from postprocessing.source_tree import require_tool
from postprocessing.source_tree import ruff_command
from postprocessing.source_tree import ruff_findings
from postprocessing.source_tree import run_commands
from postprocessing.source_tree import source_files
from postprocessing.source_tree import write_compilation_database


def write(path, text=""):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as file:
        file.write(text)
    return path


class RepositoryTreeTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = self.directory.name
        write(os.path.join(self.root, ".clang-format"), "BasedOnStyle: LLVM\n")

    def tearDown(self):
        self.directory.cleanup()

    def test_the_root_is_found_from_a_nested_directory(self):
        nested = os.path.join(self.root, "src", "tarch")
        os.makedirs(nested)
        self.assertEqual(find_repository_root(nested), self.root)

    def test_a_tree_without_style_file_has_no_root(self):
        with tempfile.TemporaryDirectory() as other:
            with mock.patch("os.path.isfile", return_value=False):
                with self.assertRaises(FileNotFoundError):
                    find_repository_root(other)

    def assert_excluded(self, *names):
        for name in names:
            with self.subTest(name=name):
                self.assertTrue(is_excluded_directory(os.path.join(self.root, name)))

    def assert_walked(self, *names):
        for name in names:
            with self.subTest(name=name):
                self.assertFalse(is_excluded_directory(os.path.join(self.root, name)))

    def test_build_trees_and_application_output_are_excluded(self):
        self.assert_excluded(
            "build",
            "build-gcc",
            "install",
            "install-Debug",
            "Euler.build",
            "Euler.generated",
            "Euler.output",
        )

    def test_caches_and_external_code_are_excluded(self):
        self.assert_excluded("CMakeFiles", "__pycache__", "node_modules", "submodules")

    def test_hidden_directories_except_the_ci_directory_are_excluded(self):
        self.assert_excluded(".git", ".ipynb_checkpoints", ".ruff_cache", ".vscode")
        self.assert_walked(".gitlab")

    def test_source_directories_are_walked(self):
        self.assert_walked("src", "api", "tests", "buildings", "installation")

    def test_a_virtual_environment_is_excluded(self):
        environment = os.path.join(self.root, "tools")
        write(os.path.join(environment, "pyvenv.cfg"))
        self.assertTrue(is_excluded_directory(environment))

    def test_the_walk_returns_the_sorted_sources_outside_excluded_directories(self):
        write(os.path.join(self.root, "src", "b.cpp"))
        write(os.path.join(self.root, "src", "a.h"))
        write(os.path.join(self.root, "src", "notes.txt"))
        write(os.path.join(self.root, "build", "src", "c.cpp"))
        write(os.path.join(self.root, "tests", "Euler.generated", "d.cpp"))
        write(os.path.join(self.root, ".cache", "e.cpp"))
        write(os.path.join(self.root, ".gitlab", "f.h"))
        self.assertEqual(
            source_files(self.root, (".cpp", ".h")),
            [
                os.path.join(self.root, ".gitlab", "f.h"),
                os.path.join(self.root, "src", "a.h"),
                os.path.join(self.root, "src", "b.cpp"),
            ],
        )

    def test_a_source_path_lies_below_the_root_and_outside_excluded_directories(self):
        self.assertTrue(
            is_source_path(self.root, os.path.join(self.root, "src", "a.cpp"))
        )
        self.assertFalse(
            is_source_path(self.root, os.path.join(self.root, "build", "a.cpp"))
        )
        self.assertFalse(
            is_source_path(self.root, os.path.join(self.root, ".cache", "a.cpp"))
        )
        self.assertFalse(
            is_source_path(self.root, os.path.join(os.path.dirname(self.root), "a.cpp"))
        )


class ToolTest(unittest.TestCase):
    def test_the_major_version_is_the_first_dotted_number(self):
        self.assertEqual(major_version("clang-format version 23.1.1"), 23)
        self.assertEqual(
            major_version("LLVM (http://llvm.org/):\n  LLVM version 22.1.8\n"), 22
        )
        self.assertEqual(major_version("black, 26.5.1 (compiled: yes)"), 26)
        self.assertEqual(major_version("no version here"), 0)

    def test_a_missing_tool_raises(self):
        with self.assertRaises(ToolError):
            require_tool("a-tool-that-does-not-exist-anywhere")

    def test_a_tool_older_than_required_raises_and_a_newer_one_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            tool = os.path.join(directory, "fake-tool")
            write(tool, f"#!{sys.executable}\nprint('fake-tool version 19.1.0')\n")
            os.chmod(tool, os.stat(tool).st_mode | stat.S_IEXEC)
            with mock.patch.dict(
                os.environ,
                {"PATH": directory + os.pathsep + os.environ.get("PATH", "")},
            ):
                with self.assertRaises(ToolError):
                    require_tool("fake-tool", 20)
                self.assertEqual(require_tool("fake-tool", 19), tool)

    def test_commands_return_their_results_in_order(self):
        commands = [[sys.executable, "-c", f"print({number})"] for number in range(5)]
        results = run_commands(commands, 3)
        self.assertEqual(
            [result.stdout.strip() for result in results], ["0", "1", "2", "3", "4"]
        )

    def test_commands_run_without_forced_colour(self):
        command = [
            sys.executable,
            "-c",
            "import os; print(os.environ.get('FORCE_COLOR'), os.environ.get('CLICOLOR_FORCE'), os.environ.get('NO_COLOR'))",
        ]
        environment = dict(os.environ, FORCE_COLOR="3", CLICOLOR_FORCE="1")
        [result] = run_commands([command], 1, environment)
        self.assertEqual(result.stdout.strip(), "None None 1")

    def test_the_number_of_jobs_is_positive(self):
        self.assertEqual(positive_integer("4"), 4)
        with self.assertRaises(argparse.ArgumentTypeError):
            positive_integer("0")


class CompilationDatabaseTest(unittest.TestCase):
    PRECOMPILED = [
        "-Winvalid-pch",
        "-Xclang",
        "-include-pch",
        "-Xclang",
        "/build/cmake_pch.hxx.pch",
        "-Xclang",
        "-include",
        "-Xclang",
        "/build/cmake_pch.hxx",
    ]

    def test_the_precompiled_header_and_werror_leave_the_arguments(self):
        entry = {
            "arguments": [
                "clang++",
                "-DDIMENSIONS=2",
                "-Werror",
                *self.PRECOMPILED,
                "-c",
                "a.cpp",
            ]
        }
        self.assertEqual(
            compilation_arguments(entry),
            [
                "clang++",
                "-DDIMENSIONS=2",
                "-Xclang",
                "-include",
                "-Xclang",
                "/build/cmake_pch.hxx",
                "-c",
                "a.cpp",
            ],
        )

    def test_a_command_string_splits_into_arguments(self):
        self.assertEqual(
            compilation_arguments({"command": "clang++ -c 'a b.cpp'"}),
            ["clang++", "-c", "a b.cpp"],
        )

    def test_each_source_below_the_root_keeps_its_first_entry(self):
        with (
            tempfile.TemporaryDirectory() as root,
            tempfile.TemporaryDirectory() as target,
        ):
            build = os.path.join(root, "build")
            source = os.path.join(root, "src", "a.cpp")
            entries = [
                {
                    "directory": build,
                    "file": source,
                    "command": "clang++ -DDIMENSIONS=2 -c " + source,
                },
                {
                    "directory": build,
                    "file": source,
                    "command": "clang++ -DDIMENSIONS=3 -c " + source,
                },
                {
                    "directory": build,
                    "file": os.path.join(build, "pch.cxx"),
                    "command": "clang++ -c pch.cxx",
                },
                {
                    "directory": build,
                    "file": "/elsewhere/b.cpp",
                    "command": "clang++ -c /elsewhere/b.cpp",
                },
            ]
            write(os.path.join(build, "compile_commands.json"), json.dumps(entries))
            units = write_compilation_database(root, build, target)
            with open(
                os.path.join(target, "compile_commands.json"), encoding="utf-8"
            ) as database:
                written = json.load(database)
        self.assertEqual(units, [source])
        self.assertEqual(len(written), 1)
        self.assertIn("-DDIMENSIONS=2", written[0]["arguments"])


class OutputTest(unittest.TestCase):
    def test_clang_warnings_and_errors_are_findings_and_notes_are_not(self):
        output = (
            "/repo/src/a.cpp:3:5: warning: use nullptr [modernize-use-nullptr]\n"
            "    int* p = 0;\n"
            "/repo/src/a.cpp:1:1: note: expanded from here\n"
            "/repo/src/b.h:7:1: error: unknown type name 'foo' [clang-diagnostic-error]\n"
        )
        self.assertEqual(
            clang_findings(output),
            [
                Finding(
                    "/repo/src/a.cpp", 3, "warning: use nullptr [modernize-use-nullptr]"
                ),
                Finding(
                    "/repo/src/b.h",
                    7,
                    "error: unknown type name 'foo' [clang-diagnostic-error]",
                ),
            ],
        )

    def test_ruff_diagnostics_are_findings_and_the_summary_is_not(self):
        output = "/repo/a.py:2:8: F401 [*] `os` imported but unused\nFound 1 error.\n"
        self.assertEqual(
            ruff_findings(output),
            [Finding("/repo/a.py", 2, "F401 [*] `os` imported but unused")],
        )

    def test_the_ruff_command_selects_ignores_and_fixes(self):
        command = ruff_command("ruff", ("E4", "F"), ("F401",), True, ["a.py"])
        self.assertEqual(
            command,
            [
                "ruff",
                "check",
                "--no-cache",
                "--output-format",
                "concise",
                "--select",
                "E4,F",
                "--ignore",
                "F401",
                "--fix",
                "a.py",
            ],
        )

    def test_the_report_prints_each_finding_once_and_fails_with_findings(self):
        findings = [
            Finding("/repo/b.py", 0, "late"),
            Finding("/repo/a.py", 3, "early"),
            Finding("/repo/a.py", 3, "early"),
        ]
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = report(findings, "/repo", "{} findings.")
        self.assertEqual(status, 1)
        self.assertEqual(output.getvalue(), "a.py:3: early\nb.py: late\n2 findings.\n")

    def test_the_report_passes_without_findings(self):
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(report([], "/repo", "{} findings."), 0)


if __name__ == "__main__":
    unittest.main()
