# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import shutil
import unittest

from postprocessing.apply_black import black_command
from postprocessing.tests.repository_fixture import TemporaryRepository

UNFORMATTED = "x = {  'a':37,'b':42}\n"


class BlackCommandTest(unittest.TestCase):
    def test_a_check_lists_the_files_black_would_change(self):
        self.assertEqual(
            black_command("black", True, 4, ["a.py"]),
            ["black", "--check", "--workers", "4", "a.py"],
        )

    def test_formatting_runs_quietly(self):
        self.assertEqual(
            black_command("black", False, 2, ["a.py"]),
            ["black", "--quiet", "--workers", "2", "a.py"],
        )


@unittest.skipUnless(shutil.which("black"), "black is not installed")
class ApplyBlackTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()
        self.repository.write("scripts/values.py", UNFORMATTED)

    def tearDown(self):
        self.repository.cleanup()

    def test_a_check_lists_the_unformatted_file_and_leaves_it_alone(self):
        result = self.repository.run("apply_black", "--check")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("scripts/values.py: black changes this file", result.stdout)
        self.assertEqual(self.repository.read("scripts/values.py"), UNFORMATTED)

    def test_formatting_rewrites_the_file(self):
        self.assertEqual(self.repository.run("apply_black").returncode, 0)
        self.assertEqual(
            self.repository.read("scripts/values.py"), 'x = {"a": 37, "b": 42}\n'
        )


if __name__ == "__main__":
    unittest.main()
