# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import os
import re
import sys

from postprocessing.apply_clang_tidy import analyse
from postprocessing.apply_clang_tidy import build_parser
from postprocessing.source_tree import CPP_SUFFIXES
from postprocessing.source_tree import Finding
from postprocessing.source_tree import ToolError
from postprocessing.source_tree import report
from postprocessing.source_tree import resolve_root
from postprocessing.source_tree import source_files

HEADER_SUFFIXES = (".cuh", ".h", ".hh", ".hpp")
INLINE_DEFINITION_SUFFIX = ".cpph"
TRANSLATION_UNIT_SUFFIXES = (".c", ".cc", ".cpp", ".cu", ".cxx")
PRECOMPILED_HEADER = "StdAfx.h"

# Includes below this directory spell their path from it, and only the precompiled header
# includes standard headers.
SOURCE_DIRECTORY = "src"

INCLUDE = re.compile(
    r'^\s*#\s*include\s*(?P<spelling>"(?P<quoted>[^"]+)"|<(?P<angled>[^>]+)>)'
)
PRAGMA_ONCE = re.compile(r"^\s*#\s*pragma\s+once\b", re.MULTILINE)

STANDARD_HEADERS = frozenset("""
    algorithm any array atomic barrier bit bitset cassert cctype cerrno cfenv cfloat charconv
    chrono cinttypes climits clocale cmath codecvt compare complex concepts condition_variable
    coroutine csetjmp csignal cstdarg cstddef cstdint cstdio cstdlib cstring ctime cuchar cwchar
    cwctype debugging deque exception execution expected filesystem flat_map flat_set format
    forward_list fstream functional future generator initializer_list inplace_vector iomanip ios
    iosfwd iostream istream iterator latch limits linalg list locale map mdspan memory
    memory_resource mutex new numbers numeric optional ostream print queue random ranges ratio
    regex scoped_allocator semaphore set shared_mutex source_location span spanstream sstream
    stack stacktrace stdexcept stdfloat stop_token streambuf string string_view syncstream
    system_error text_encoding thread tuple type_traits typeindex typeinfo unordered_map
    unordered_set utility valarray variant vector version
    assert.h complex.h ctype.h errno.h fenv.h float.h inttypes.h iso646.h limits.h locale.h
    math.h setjmp.h signal.h stdalign.h stdarg.h stdatomic.h stdbool.h stdckdint.h stddef.h
    stdint.h stdio.h stdlib.h stdnoreturn.h string.h tgmath.h threads.h time.h uchar.h wchar.h
    wctype.h
    """.split())


def include_findings(root: str, path: str, text: str) -> list[Finding]:
    """Returns the violations of the include conventions in the text of one C or C++ file."""
    findings = []
    in_source_directory = not os.path.relpath(
        path, os.path.join(root, SOURCE_DIRECTORY)
    ).startswith(os.pardir)
    stem, suffix = os.path.splitext(os.path.basename(path))
    seen = set()
    for number, line in enumerate(text.splitlines(), start=1):
        match = INCLUDE.match(line)
        if match is None:
            continue
        spelling = match.group("spelling")
        quoted = match.group("quoted")
        if spelling in seen:
            findings.append(Finding(path, number, f"includes {spelling} a second time"))
        seen.add(spelling)
        if quoted is not None and quoted.split("/")[0] == os.pardir:
            findings.append(
                Finding(path, number, f"includes {spelling} through a parent directory")
            )
        if not in_source_directory:
            continue
        if (
            match.group("angled") in STANDARD_HEADERS
            and os.path.basename(path) != PRECOMPILED_HEADER
        ):
            findings.append(
                Finding(
                    path,
                    number,
                    f"includes the standard header {spelling} outside {PRECOMPILED_HEADER}",
                )
            )
        own_header = (
            suffix in TRANSLATION_UNIT_SUFFIXES
            and os.path.splitext(quoted or "")[0] == stem
        )
        if quoted is not None and "/" not in quoted and not own_header:
            findings.append(
                Finding(
                    path,
                    number,
                    f"spells {spelling} without its path from {SOURCE_DIRECTORY}",
                )
            )
    has_pragma_once = PRAGMA_ONCE.search(text) is not None
    if suffix in HEADER_SUFFIXES and not has_pragma_once:
        findings.append(Finding(path, 0, "a header needs a #pragma once"))
    if suffix == INLINE_DEFINITION_SUFFIX and has_pragma_once:
        findings.append(
            Finding(path, 0, "an inline definition file must not contain #pragma once")
        )
    return findings


INCLUDE_CLEANER_ARGUMENTS = ["--checks=-*,misc-include-cleaner"]

# Standard headers reach every file through the precompiled header.
PRECOMPILED_SYMBOL = re.compile(
    rf'no header providing "std::|included header {re.escape(PRECOMPILED_HEADER)} '
)


def include_cleaner_findings(findings: list[Finding]) -> list[Finding]:
    """Returns the unused and indirect includes clang-tidy reports, without the standard library ones."""
    return [
        finding
        for finding in findings
        if PRECOMPILED_SYMBOL.search(finding.message) is None
    ]


def main() -> None:
    parser = build_parser(
        "Checks the include conventions of every C and C++ file of the repository and, with a "
        "compilation database, reports unused and indirect includes through clang-tidy."
    )
    parser.add_argument(
        "--conventions-only",
        action="store_true",
        help="Skips the clang-tidy analysis. Only that analysis reads the compilation database",
    )
    args = parser.parse_args()
    try:
        root = resolve_root(args.root)
    except FileNotFoundError as error:
        print(error)
        sys.exit(2)

    findings = []
    files = source_files(root, CPP_SUFFIXES)
    for path in files:
        with open(path, encoding="utf-8", errors="replace") as source:
            findings.extend(include_findings(root, path, source.read()))

    build_directory = os.path.abspath(
        args.build_directory or os.path.join(root, "build")
    )
    database = os.path.join(build_directory, "compile_commands.json")
    if not args.conventions_only and not os.path.isfile(database):
        print(
            f"The clang-tidy analysis reads the compilation database {database}. "
            "This run checks the include conventions only."
        )
    elif not args.conventions_only:
        try:
            cleaner_findings, _ = analyse(
                root,
                build_directory,
                args.jobs,
                lambda index: INCLUDE_CLEANER_ARGUMENTS,
            )
        except ToolError as error:
            print(error)
            sys.exit(2)
        findings.extend(include_cleaner_findings(cleaner_findings))
    sys.exit(
        report(
            findings,
            root,
            "{} include findings in " + str(len(files)) + " C and C++ files.",
        )
    )


if __name__ == "__main__":
    main()
