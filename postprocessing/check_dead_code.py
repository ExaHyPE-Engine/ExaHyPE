# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import os
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

DEFAULT_CONFIDENCE = 80
VULTURE_FOUND_DEAD_CODE = 3
VULTURE_DIAGNOSTIC = re.compile(
    r"^(?P<path>[^\s:][^:]*):(?P<line>\d+): (?P<message>.+)$", re.MULTILINE
)


def confidence(value: str) -> int:
    number = int(value)
    if not 0 <= number <= 100:
        raise argparse.ArgumentTypeError("The confidence is a percentage.")
    return number


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reports unused functions, classes, variables, imports and unreachable code in "
        "every Python file of the repository with vulture."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--min-confidence",
        type=confidence,
        default=DEFAULT_CONFIDENCE,
        help=f"Smallest confidence in percent a reported finding has (default: {DEFAULT_CONFIDENCE})",
    )
    return parser


def vulture_command(
    executable: str, minimum_confidence: int, files: list[str]
) -> list[str]:
    return [executable, "--min-confidence", str(minimum_confidence), *files]


def vulture_findings(output: str) -> list[Finding]:
    """Returns the unused code vulture reports, one finding per line of its output."""
    return [
        Finding(
            os.path.abspath(match.group("path")),
            int(match.group("line")),
            match.group("message"),
        )
        for match in VULTURE_DIAGNOSTIC.finditer(output)
    ]


def main() -> None:
    args = build_parser().parse_args()
    try:
        root = resolve_root(args.root)
        vulture = require_tool("vulture")
    except (FileNotFoundError, ToolError) as error:
        print(error)
        sys.exit(2)

    files = source_files(root, PYTHON_SUFFIXES)
    [result] = run_commands([vulture_command(vulture, args.min_confidence, files)], 1)
    if result.returncode not in (0, VULTURE_FOUND_DEAD_CODE):
        print((result.stderr or result.stdout).strip())
        sys.exit(2)
    sys.exit(
        report(
            vulture_findings(result.stdout),
            root,
            "{} unused code findings in " + str(len(files)) + " Python files.",
        )
    )


if __name__ == "__main__":
    main()
