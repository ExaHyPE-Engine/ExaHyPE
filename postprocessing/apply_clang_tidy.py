# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile

from collections.abc import Callable

from postprocessing.source_tree import Finding
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import add_common_arguments
from postprocessing.source_tree import clang_findings
from postprocessing.source_tree import report
from postprocessing.source_tree import require_tool
from postprocessing.source_tree import resolve_root
from postprocessing.source_tree import run_commands
from postprocessing.source_tree import write_compilation_database

MINIMUM_MAJOR_VERSION = 20


def build_parser(description: str) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description)
    add_common_arguments(parser)
    parser.add_argument(
        "--build-directory",
        help="CMake build directory whose compile_commands.json lists the translation units (default: build below the root)",
    )
    return parser


def tidy_command(
    executable: str, database_directory: str, path: str, arguments: list[str]
) -> list[str]:
    """Returns the clang-tidy call that analyses one translation unit of the filtered database."""
    return [executable, "-p", database_directory, "--quiet", *arguments, path]


def analyse(
    root: str,
    build_directory: str,
    jobs: int,
    arguments_of: Callable[[int], list[str]],
) -> tuple[list[Finding], list[str]]:
    """Runs clang-tidy on every translation unit below root that the build directory compiles.

    ``arguments_of`` maps the index of a translation unit to the options of its call. Returns
    the findings and the translation units.
    """
    executable = require_tool("clang-tidy", MINIMUM_MAJOR_VERSION)
    with tempfile.TemporaryDirectory() as database_directory:
        units = write_compilation_database(root, build_directory, database_directory)
        commands = [
            tidy_command(executable, database_directory, path, arguments_of(index))
            for index, path in enumerate(units)
        ]
        results = run_commands(commands, jobs)
    findings = []
    for path, result in zip(units, results):
        unit_findings = clang_findings(result.stdout)
        if result.returncode != 0 and not unit_findings:
            last_line = (result.stderr.strip().splitlines() or ["no output"])[-1]
            unit_findings = [Finding(path, 0, f"clang-tidy fails: {last_line}")]
        findings.extend(unit_findings)
    return findings, units


def main() -> None:
    parser = build_parser(
        "Runs clang-tidy with the repository configuration on every translation unit of a CMake "
        "build and on the headers it includes."
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Applies the fixes clang-tidy suggests and formats the changed code",
    )
    args = parser.parse_args()
    try:
        root = resolve_root(args.root)
        build_directory = os.path.abspath(
            args.build_directory or os.path.join(root, "build")
        )
        if not args.fix:
            findings, units = analyse(
                root, build_directory, args.jobs, lambda index: []
            )
            sys.exit(
                report(
                    findings,
                    root,
                    "{} clang-tidy diagnostics in "
                    + str(len(units))
                    + " translation units.",
                )
            )
        applier = require_tool("clang-apply-replacements", MINIMUM_MAJOR_VERSION)
        with tempfile.TemporaryDirectory() as fixes_directory:
            findings, units = analyse(
                root,
                build_directory,
                args.jobs,
                lambda index: [
                    f"--export-fixes={os.path.join(fixes_directory, f'{index}.yaml')}"
                ],
            )
            # The replacements of a header that several translation units include merge once.
            subprocess.run(
                [applier, "--format", f"--style-config={root}", fixes_directory],
                check=True,
            )
    except (FileNotFoundError, ToolError, subprocess.CalledProcessError) as error:
        print(error)
        sys.exit(2)
    print(
        f"Applied the fixes of {len(set(findings))} diagnostics in {len(units)} translation units."
    )


if __name__ == "__main__":
    main()
