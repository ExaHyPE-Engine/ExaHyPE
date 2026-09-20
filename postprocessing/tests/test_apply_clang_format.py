# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import shutil
import unittest

from postprocessing.apply_clang_format import build_parser
from postprocessing.apply_clang_format import format_command
from postprocessing.tests.repository_fixture import TemporaryRepository

UNFORMATTED = "int    main( ) {return 0;}\n"


class FormatCommandTest(unittest.TestCase):
    def test_a_check_changes_no_file_and_fails_for_a_change(self):
        self.assertEqual(
            format_command("clang-format", "/repo/.clang-format", True, "/repo/a.cpp"),
            [
                "clang-format",
                "--style=file:/repo/.clang-format",
                "--dry-run",
                "--Werror",
                "/repo/a.cpp",
            ],
        )

    def test_formatting_rewrites_the_file_in_place(self):
        self.assertEqual(
            format_command("clang-format", "/repo/.clang-format", False, "/repo/a.cpp"),
            ["clang-format", "--style=file:/repo/.clang-format", "-i", "/repo/a.cpp"],
        )

    def test_the_parser_formats_by_default(self):
        self.assertFalse(build_parser().parse_args([]).check)
        self.assertTrue(build_parser().parse_args(["--check"]).check)


@unittest.skipUnless(shutil.which("clang-format"), "clang-format is not installed")
class ApplyClangFormatTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()
        self.repository.write("src/main.cpp", UNFORMATTED)
        self.repository.write("build/generated.cpp", UNFORMATTED)

    def tearDown(self):
        self.repository.cleanup()

    def test_a_check_lists_the_unformatted_file_and_leaves_it_alone(self):
        result = self.repository.run("apply_clang_format", "--check")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("src/main.cpp: clang-format changes this file", result.stdout)
        self.assertEqual(self.repository.read("src/main.cpp"), UNFORMATTED)

    def test_formatting_rewrites_the_sources_and_skips_build_directories(self):
        self.assertEqual(self.repository.run("apply_clang_format").returncode, 0)
        self.assertNotEqual(self.repository.read("src/main.cpp"), UNFORMATTED)
        self.assertEqual(self.repository.read("build/generated.cpp"), UNFORMATTED)
        self.assertEqual(
            self.repository.run("apply_clang_format", "--check").returncode, 0
        )


if __name__ == "__main__":
    unittest.main()
