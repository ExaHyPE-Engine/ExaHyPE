# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import sys

from postprocessing.check_imports import IMPORT_RULES
from postprocessing.source_tree import PYTHON_SUFFIXES
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import add_common_arguments
from postprocessing.source_tree import report
from postprocessing.source_tree import require_tool
from postprocessing.source_tree import resolve_root
from postprocessing.source_tree import ruff_check
from postprocessing.source_tree import source_files

# Syntax errors, undefined and unused names and misplaced statements. check_imports reports
# the import rules.
LINT_RULES = ("E4", "E7", "E9", "F")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Checks every Python file of the repository with the pyflakes and "
        "pycodestyle error rules of ruff."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Applies the fixes ruff can apply safely",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()
    try:
        root = resolve_root(args.root)
        files = source_files(root, PYTHON_SUFFIXES)
        findings = ruff_check(
            require_tool("ruff"), LINT_RULES, IMPORT_RULES, args.fix, files
        )
    except (FileNotFoundError, ToolError) as error:
        print(error)
        sys.exit(2)
    sys.exit(
        report(
            findings, root, "{} lint findings in " + str(len(files)) + " Python files."
        )
    )


if __name__ == "__main__":
    main()
