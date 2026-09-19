# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import shutil
import unittest

from postprocessing.check_imports import IMPORT_RULES
from postprocessing.check_python_lint import LINT_RULES
from postprocessing.check_python_lint import build_parser
from postprocessing.tests.repository_fixture import TemporaryRepository


class LintRulesTest(unittest.TestCase):
    def test_the_lint_rules_cover_errors_and_pyflakes(self):
        self.assertEqual(LINT_RULES, ("E4", "E7", "E9", "F"))
        self.assertIn("F401", IMPORT_RULES)

    def test_the_parser_checks_without_fixing_by_default(self):
        self.assertFalse(build_parser().parse_args([]).fix)


@unittest.skipUnless(shutil.which("ruff"), "ruff is not installed")
class CheckPythonLintScriptTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()

    def tearDown(self):
        self.repository.cleanup()

    def test_an_undefined_name_fails_the_script(self):
        self.repository.write("tools/run.py", "print(undefined_value)\n")
        result = self.repository.run("check_python_lint")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("tools/run.py:1: F821", result.stdout)

    def test_an_unused_import_is_left_to_the_import_check(self):
        self.repository.write("tools/run.py", "import os\n")
        result = self.repository.run("check_python_lint")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
