# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

from postprocessing.check_includes import include_cleaner_findings
from postprocessing.check_includes import include_findings
from postprocessing.source_tree import Finding
from postprocessing.tests.repository_fixture import TemporaryRepository

ROOT = "/repo"


def messages(path, text):
    return [
        (finding.line, finding.message)
        for finding in include_findings(ROOT, path, text)
    ]


class IncludeConventionTest(unittest.TestCase):
    def test_a_header_with_pragma_once_and_full_paths_passes(self):
        text = '#pragma once\n\n#include "tarch/Assertions.h"\n#include <mpi.h>\n'
        self.assertEqual(messages("/repo/src/tarch/Foo.h", text), [])

    def test_a_translation_unit_includes_its_own_header_bare(self):
        self.assertEqual(messages("/repo/src/tarch/Foo.cpp", '#include "Foo.h"\n'), [])

    def test_a_second_include_of_a_header_is_reported(self):
        text = '#pragma once\n#include "tarch/A.h"\n#include "tarch/A.h"\n'
        self.assertEqual(
            messages("/repo/src/tarch/Foo.h", text),
            [(3, 'includes "tarch/A.h" a second time')],
        )

    def test_a_parent_directory_include_is_reported_everywhere(self):
        text = '#include "../riemannSolvers/Routines.h"\n'
        self.assertEqual(
            messages("/repo/applications/seis/Solver.cpp", text),
            [(1, 'includes "../riemannSolvers/Routines.h" through a parent directory')],
        )

    def test_an_inline_definition_file_includes_nothing_and_holds_no_pragma_once(self):
        text = '#pragma once\n#include "tarch/A.h"\n'
        self.assertEqual(
            messages("/repo/src/tarch/Foo.cpph", text),
            [
                (2, "an inline definition file includes a header"),
                (0, "an inline definition file holds a #pragma once"),
            ],
        )

    def test_a_header_without_pragma_once_is_reported(self):
        self.assertEqual(
            messages("/repo/tests/fv/PDE.h", "int f();\n"),
            [(0, "a header holds no #pragma once")],
        )

    def test_a_standard_header_below_src_belongs_to_the_precompiled_header(self):
        self.assertEqual(
            messages("/repo/src/tarch/Foo.cpp", "#include <vector>\n"),
            [(1, "includes the standard header <vector> outside StdAfx.h")],
        )
        self.assertEqual(
            messages("/repo/src/tarch/StdAfx.h", "#pragma once\n#include <vector>\n"),
            [],
        )
        self.assertEqual(
            messages("/repo/tests/fv/Solver.cpp", "#include <vector>\n"), []
        )

    def test_a_bare_include_below_src_is_reported(self):
        self.assertEqual(
            messages(
                "/repo/src/tarch/plotter/CellTypes.h",
                '#pragma once\n#include "CellTypes.cpph"\n',
            ),
            [(2, 'spells "CellTypes.cpph" without its path from src')],
        )


class IncludeCleanerTest(unittest.TestCase):
    def test_standard_library_symbols_and_the_precompiled_header_are_dropped(self):
        findings = [
            Finding(
                "/repo/src/a.cpp",
                1,
                'warning: no header providing "std::vector" is directly included [misc-include-cleaner]',
            ),
            Finding(
                "/repo/src/a.cpp",
                2,
                "warning: included header StdAfx.h is not used directly [misc-include-cleaner]",
            ),
            Finding(
                "/repo/src/a.cpp",
                3,
                'warning: no header providing "assertion" is directly included [misc-include-cleaner]',
            ),
        ]
        self.assertEqual(include_cleaner_findings(findings), findings[2:])


class CheckIncludesScriptTest(unittest.TestCase):
    def setUp(self):
        self.repository = TemporaryRepository()

    def tearDown(self):
        self.repository.cleanup()

    def test_a_violation_fails_the_script(self):
        self.repository.write("src/tarch/Foo.h", '#include "../Bar.h"\n')
        result = self.repository.run("check_includes", "--conventions-only")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("src/tarch/Foo.h: a header holds no #pragma once", result.stdout)

    def test_a_clean_repository_passes(self):
        self.repository.write(
            "src/tarch/Foo.h", '#pragma once\n#include "tarch/Bar.h"\n'
        )
        result = self.repository.run("check_includes", "--conventions-only")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
