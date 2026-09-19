# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import shutil
import unittest

from postprocessing.check_dead_code import confidence
from postprocessing.check_dead_code import vulture_command
from postprocessing.check_dead_code import vulture_findings
from postprocessing.source_tree import Finding
from postprocessing.tests.repository_fixture import TemporaryRepository


class VultureTest(unittest.TestCase):
    def test_the_command_passes_the_confidence(self):
        self.assertEqual(
            vulture_command("vulture", 80, ["a.py"]),
            ["vulture", "--min-confidence", "80", "a.py"],
        )

    def test_each_output_line_is_a_finding(self):
        output = "/repo/a.py:3: unused import 'os' (90% confidence)\n"
        self.assertEqual(
            vulture_findings(output),
            [Finding("/repo/a.py", 3, "unused import 'os' (90% confidence)")],
        )

    def test_the_confidence_is_a_percentage(self):
        self.assertEqual(confidence("60"), 60)
        with self.assertRaises(argparse.ArgumentTypeError):
            confidence("101")


@unittest.skipUnless(shutil.which("vulture"), "vulture is not installed")
class CheckDeadCodeScriptTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()

    def tearDown(self):
        self.repository.cleanup()

    def test_unreachable_code_fails_the_script(self):
        self.repository.write(
            "tools/run.py",
            "def value():\n    return 1\n    print('never')\n\n\nvalue()\n",
        )
        result = self.repository.run("check_dead_code")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("tools/run.py:3: unreachable code", result.stdout)

    def test_used_code_passes(self):
        self.repository.write(
            "tools/run.py", "def value():\n    return 1\n\n\nprint(value())\n"
        )
        self.assertEqual(self.repository.run("check_dead_code").returncode, 0)


if __name__ == "__main__":
    unittest.main()
