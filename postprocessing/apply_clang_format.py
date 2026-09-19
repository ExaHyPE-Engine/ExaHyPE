# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import os
import sys

from postprocessing.source_tree import CPP_SUFFIXES
from postprocessing.source_tree import STYLE_FILE
from postprocessing.source_tree import Finding
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import add_common_arguments
from postprocessing.source_tree import report
from postprocessing.source_tree import require_tool
from postprocessing.source_tree import resolve_root
from postprocessing.source_tree import run_commands
from postprocessing.source_tree import source_files

MINIMUM_MAJOR_VERSION = 20
FORMAT_VIOLATION = "[-Wclang-format-violations]"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Formats every C and C++ source file of the repository with clang-format."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Lists the files clang-format would change and changes none",
    )
    return parser


def format_command(
    executable: str, style_file: str, check: bool, path: str
) -> list[str]:
    """Returns the clang-format call that rewrites a file, or that fails for a file it would change."""
    mode = ["--dry-run", "--Werror"] if check else ["-i"]
    return [executable, f"--style=file:{style_file}", *mode, path]


def main() -> None:
    args = build_parser().parse_args()
    try:
        root = resolve_root(args.root)
        executable = require_tool("clang-format", MINIMUM_MAJOR_VERSION)
    except (FileNotFoundError, ToolError) as error:
        print(error)
        sys.exit(2)

    style_file = os.path.join(root, STYLE_FILE)
    files = source_files(root, CPP_SUFFIXES)
    results = run_commands(
        [format_command(executable, style_file, args.check, path) for path in files],
        args.jobs,
    )
    findings = []
    errors = []
    for path, result in zip(files, results):
        if args.check and result.returncode != 0 and FORMAT_VIOLATION in result.stderr:
            findings.append(Finding(path, 0, "clang-format changes this file"))
        elif result.returncode != 0:
            errors.append(f"{os.path.relpath(path, root)}: {result.stderr.strip()}")

    for error in errors:
        print(error)
    if args.check:
        status = report(
            findings,
            root,
            "{} of " + str(len(files)) + " C and C++ files need formatting.",
        )
        sys.exit(2 if errors else status)
    print(f"Formatted {len(files) - len(errors)} of {len(files)} C and C++ files.")
    sys.exit(2 if errors else 0)


if __name__ == "__main__":
    main()
