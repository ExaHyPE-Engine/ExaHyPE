# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import re
import sys

from postprocessing.source_tree import PYTHON_SUFFIXES
from postprocessing.source_tree import Finding
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import add_common_arguments
from postprocessing.source_tree import report
from postprocessing.source_tree import require_tool
from postprocessing.source_tree import resolve_root
from postprocessing.source_tree import run_commands
from postprocessing.source_tree import source_files

WOULD_REFORMAT = re.compile(r"^would reformat (?P<path>.+)$", re.MULTILINE)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Formats every Python file of the repository with black."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Lists the files black would change and changes none",
    )
    return parser


def black_command(
    executable: str, check: bool, jobs: int, files: list[str]
) -> list[str]:
    """Returns the black call that rewrites the files, or that lists the files it would change."""
    mode = ["--check"] if check else ["--quiet"]
    return [executable, *mode, "--workers", str(jobs), *files]


def main() -> None:
    args = build_parser().parse_args()
    try:
        root = resolve_root(args.root)
        black = require_tool("black")
    except (FileNotFoundError, ToolError) as error:
        print(error)
        sys.exit(2)

    files = source_files(root, PYTHON_SUFFIXES)
    [result] = run_commands([black_command(black, args.check, args.jobs, files)], 1)
    findings = [
        Finding(match.group("path"), 0, "black changes this file")
        for match in WOULD_REFORMAT.finditer(result.stderr)
    ]
    # Black also leaves with status one when it crashes, so only the listed files count.
    if result.returncode != 0 and not (
        args.check and result.returncode == 1 and findings
    ):
        print(result.stderr.strip())
        sys.exit(2)
    if args.check:
        sys.exit(
            report(
                findings,
                root,
                "{} of " + str(len(files)) + " Python files need formatting.",
            )
        )
    print(f"Formatted {len(files)} Python files.")


if __name__ == "__main__":
    main()
