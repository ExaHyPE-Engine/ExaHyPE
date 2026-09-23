# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import json
import os
import shutil
import unittest

from postprocessing.apply_clang_tidy import analyse
from postprocessing.apply_clang_tidy import build_parser
from postprocessing.apply_clang_tidy import tidy_command
from postprocessing.tests.repository_fixture import TemporaryRepository

SOURCE = "int* pointer = 0;\n"


class TidyCommandTest(unittest.TestCase):
    def test_a_translation_unit_is_analysed_quietly_with_the_filtered_database(self):
        self.assertEqual(
            tidy_command("clang-tidy", "/tmp/database", "/repo/src/a.cpp", ["--fix"]),
            [
                "clang-tidy",
                "-p",
                "/tmp/database",
                "--quiet",
                "--fix",
                "/repo/src/a.cpp",
            ],
        )

    def test_the_build_directory_is_an_option(self):
        arguments = build_parser("description").parse_args(
            ["--build-directory", "build-gcc"]
        )
        self.assertEqual(arguments.build_directory, "build-gcc")


@unittest.skipUnless(
    shutil.which("clang-tidy") and shutil.which("clang-apply-replacements"),
    "clang-tidy is not installed",
)
class ApplyClangTidyTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()
        self.repository.write(".clang-tidy", "Checks: '-*,modernize-use-nullptr'\n")
        self.source = self.repository.write("src/pointer.cpp", SOURCE)
        self.build = os.path.join(self.repository.root, "build")
        entry = {
            "directory": self.build,
            "file": self.source,
            "arguments": ["clang++", "-std=c++17", "-c", self.source],
        }
        self.repository.write("build/compile_commands.json", json.dumps([entry]))

    def tearDown(self):
        self.repository.cleanup()

    def test_the_analysis_reports_the_diagnostic_of_the_translation_unit(self):
        findings, units = analyse(self.repository.root, self.build, 1, lambda index: [])
        self.assertEqual(units, [self.source])
        self.assertEqual([finding.line for finding in findings], [1])
        self.assertIn("modernize-use-nullptr", findings[0].message)

    def test_the_script_fails_with_a_diagnostic_and_fixes_it_on_request(self):
        self.assertEqual(self.repository.run("apply_clang_tidy").returncode, 1)
        self.assertEqual(self.repository.read("src/pointer.cpp"), SOURCE)
        result = self.repository.run("apply_clang_tidy", "--fix")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("nullptr", self.repository.read("src/pointer.cpp"))


if __name__ == "__main__":
    unittest.main()
