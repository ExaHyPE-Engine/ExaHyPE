# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import ast
import os
import shutil
import unittest

from postprocessing.check_imports import attribute_chains
from postprocessing.check_imports import fresh_interpreter_findings
from postprocessing.check_imports import import_findings
from postprocessing.check_imports import package_modules
from postprocessing.check_imports import stale_submodule_imports
from postprocessing.check_imports import top_level_packages
from postprocessing.source_tree import PYTHON_SUFFIXES
from postprocessing.source_tree import source_files
from postprocessing.tests.repository_fixture import TemporaryRepository


class StaleImportTest(unittest.TestCase):
    def test_attribute_chains_contain_the_full_dotted_names(self):
        tree = ast.parse("import os.path\nos.path.join('a', 'b')\nvalue = name\n")
        self.assertEqual(attribute_chains(tree), {"os.path.join", "value", "name"})

    def test_a_submodule_import_the_file_never_reads_is_stale(self):
        findings = stale_submodule_imports(
            "a.py", "import os.path\nimport xml.dom\nxml.dom.getDOMImplementation()\n"
        )
        self.assertEqual(
            [(finding.line, finding.message) for finding in findings],
            [(1, "import os.path is unused")],
        )

    def test_aliased_and_plain_imports_are_left_to_ruff(self):
        self.assertEqual(
            stale_submodule_imports("a.py", "import os\nimport xml.dom as dom\n"), []
        )

    def test_a_file_that_does_not_parse_is_reported(self):
        findings = stale_submodule_imports("a.py", "import (\n")
        self.assertEqual(len(findings), 1)
        self.assertIn("cannot parse the file", findings[0].message)


class PackageTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()
        self.repository.write("api/solver/__init__.py", "")
        self.repository.write("api/solver/terms.py", "VALUE = 1\n")
        self.repository.write(
            "api/solver/family/__init__.py", "from ..terms import VALUE\n"
        )
        # A script in a plain subdirectory writes a file when it runs. The file shows an import.
        marker = os.path.join(self.repository.root, "script-ran")
        self.repository.write(
            "api/solver/tests/generate.py", f"open({marker!r}, 'w').close()\n"
        )
        self.repository.write(
            "api/solver/broken.py", "from solver import missing_name\n"
        )
        self.repository.write(
            "api/solver/optional.py", "import a_module_outside_the_repository\n"
        )
        self.marker = marker
        self.files = source_files(self.repository.root, PYTHON_SUFFIXES)
        self.package = os.path.join(self.repository.root, "api", "solver")

    def tearDown(self):
        self.repository.cleanup()

    def test_only_a_directory_under_no_package_is_a_top_level_package(self):
        self.assertEqual(
            top_level_packages(self.repository.root, self.files), [self.package]
        )

    def test_a_script_of_a_plain_subdirectory_is_no_module(self):
        names = [name for name, _ in package_modules(self.package, self.files)]
        self.assertEqual(
            names,
            [
                "solver",
                "solver.broken",
                "solver.family",
                "solver.optional",
                "solver.terms",
            ],
        )

    def test_a_failing_import_is_reported_and_a_missing_external_module_is_not(self):
        findings = fresh_interpreter_findings(self.repository.root, self.files, 2)
        self.assertEqual(
            [os.path.basename(finding.path) for finding in findings], ["broken.py"]
        )
        self.assertIn("ImportError", findings[0].message)
        self.assertFalse(os.path.exists(self.marker))

    def test_a_missing_module_of_the_repository_is_a_finding(self):
        output = "Traceback (most recent call last):\nModuleNotFoundError: No module named 'solver.gone'\n"
        self.assertEqual(
            len(import_findings(output, "solver.family", "family.py", {"solver"})), 1
        )
        self.assertEqual(
            import_findings(
                output.replace("solver.gone", "numpy"), "solver", "a.py", {"solver"}
            ),
            [],
        )


class CheckImportsScriptTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()

    def tearDown(self):
        self.repository.cleanup()

    @unittest.skipUnless(shutil.which("ruff"), "ruff is not installed")
    def test_an_unused_import_fails_the_script(self):
        self.repository.write("tools/run.py", "import os\n")
        result = self.repository.run("check_imports")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("tools/run.py:1: F401", result.stdout)


if __name__ == "__main__":
    unittest.main()
