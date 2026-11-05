# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import json
import os
import re
import shlex
import shutil
import subprocess

CPP_SUFFIXES = (
    ".c",
    ".cc",
    ".cpp",
    ".cpph",
    ".cu",
    ".cuh",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
)
PYTHON_SUFFIXES = (".py",)
STYLE_FILE = ".clang-format"

SOURCE_HIDDEN_DIRECTORIES = frozenset({".gitlab"})
CACHE_DIRECTORIES = frozenset({"CMakeFiles", "__pycache__"})
EXTERNAL_DIRECTORIES = frozenset({"node_modules", "submodules"})
BUILD_DIRECTORY = re.compile(r"^(build|install)(-.+)?$")
APPLICATION_OUTPUT_DIRECTORY = re.compile(r"\.(build|generated|output)$")
VIRTUAL_ENVIRONMENT_MARKER = "pyvenv.cfg"

PRECOMPILED_HEADER_ARGUMENTS = ("-Winvalid-pch", "-Werror")


class ToolError(RuntimeError):
    """Raised when an external tool is missing or older than a script requires."""


@dataclasses.dataclass(frozen=True, order=True)
class Finding:
    """One diagnostic of a check at a line of a file. Line zero addresses the whole file."""

    path: str
    line: int
    message: str

    def render(self, root: str) -> str:
        location = os.path.relpath(self.path, root)
        return (
            f"{location}:{self.line}: {self.message}"
            if self.line > 0
            else f"{location}: {self.message}"
        )


def find_repository_root(start: str) -> str:
    """Returns the closest directory at or above start that contains the clang-format style file."""
    directory = os.path.abspath(start)
    while not os.path.isfile(os.path.join(directory, STYLE_FILE)):
        parent = os.path.dirname(directory)
        if parent == directory:
            raise FileNotFoundError(
                f"A directory at or above {start} must contain {STYLE_FILE}."
            )
        directory = parent
    return directory


def is_excluded_directory(path: str) -> bool:
    """Returns whether a directory contains tool state, build output, external code or a virtual environment."""
    name = os.path.basename(os.path.abspath(path))
    if name.startswith("."):
        return name not in SOURCE_HIDDEN_DIRECTORIES
    return (
        name in CACHE_DIRECTORIES
        or name in EXTERNAL_DIRECTORIES
        or BUILD_DIRECTORY.match(name) is not None
        or APPLICATION_OUTPUT_DIRECTORY.search(name) is not None
        or os.path.isfile(os.path.join(path, VIRTUAL_ENVIRONMENT_MARKER))
    )


def is_source_path(root: str, path: str) -> bool:
    """Returns whether a path lies below root and outside every excluded directory."""
    root_directory = os.path.abspath(root)
    relative = os.path.relpath(os.path.abspath(path), root_directory)
    if relative == os.curdir or relative.startswith(os.pardir):
        return False
    parents = relative.split(os.sep)[:-1]
    return not any(
        is_excluded_directory(os.path.join(root_directory, *parents[: depth + 1]))
        for depth in range(len(parents))
    )


def source_files(root: str, suffixes: tuple[str, ...]) -> list[str]:
    """Returns the sorted absolute paths of the files below root that end with one of the suffixes."""
    result = []
    for directory, subdirectories, files in os.walk(os.path.abspath(root)):
        subdirectories[:] = [
            name
            for name in subdirectories
            if not is_excluded_directory(os.path.join(directory, name))
        ]
        result.extend(
            os.path.join(directory, name) for name in files if name.endswith(suffixes)
        )
    return sorted(result)


def major_version(version_output: str) -> int:
    """Returns the major number of the first dotted version a tool prints, or zero without one."""
    match = re.search(r"(\d+)\.\d+", version_output)
    return int(match.group(1)) if match else 0


def require_tool(name: str, minimum_major_version: int = 0) -> str:
    """Returns the path of an executable whose major version is at least the given one."""
    path = shutil.which(name)
    if path is None:
        raise ToolError(
            f"{name} is not installed; pip install -r requirements.txt provides it."
        )
    if minimum_major_version > 0:
        version_output = subprocess.run(
            [path, "--version"], capture_output=True, text=True, check=False
        ).stdout
        version = major_version(version_output)
        if version < minimum_major_version:
            raise ToolError(
                f"{name} {version} is older than the required version {minimum_major_version}."
            )
    return path


def run_commands(
    commands: list[list[str]], jobs: int, environment: dict[str, str] | None = None
) -> list[subprocess.CompletedProcess]:
    """Runs the commands on up to jobs threads and returns the results in the order of the commands.

    The commands run without forced colour. The parsers of their output then read plain text.
    """
    plain_environment = dict(os.environ if environment is None else environment)
    plain_environment.pop("FORCE_COLOR", None)
    plain_environment.pop("CLICOLOR_FORCE", None)
    plain_environment["NO_COLOR"] = "1"

    def run(command: list[str]) -> subprocess.CompletedProcess:
        return subprocess.run(
            command, capture_output=True, text=True, check=False, env=plain_environment
        )

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as executor:
        return list(executor.map(run, commands))


def positive_integer(value: str) -> int:
    number = int(value)
    if number < 1:
        raise argparse.ArgumentTypeError("The value has to be positive.")
    return number


def add_common_arguments(parser: argparse.ArgumentParser) -> None:
    """Adds the repository root and the number of parallel jobs to a parser."""
    parser.add_argument(
        "--root",
        help=f"Repository to analyse (default: the closest directory above the working directory that contains {STYLE_FILE})",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=positive_integer,
        default=os.cpu_count() or 1,
        help="Number of tool processes that run at the same time (default: the number of processors)",
    )


def resolve_root(root_argument: str | None) -> str:
    return (
        os.path.abspath(root_argument)
        if root_argument
        else find_repository_root(os.getcwd())
    )


def compilation_arguments(entry: dict) -> list[str]:
    """Returns the compiler arguments of a compilation database entry without the precompiled header."""
    arguments = (
        entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
    )
    result = []
    index = 0
    while index < len(arguments):
        if (
            arguments[index : index + 4 : 2] == ["-Xclang", "-Xclang"]
            and arguments[index + 1] == "-include-pch"
        ):
            index += 4
        elif arguments[index] in PRECOMPILED_HEADER_ARGUMENTS:
            index += 1
        else:
            result.append(arguments[index])
            index += 1
    return result


def write_compilation_database(
    root: str, build_directory: str, target_directory: str
) -> list[str]:
    """Writes one entry per source file below root of the database of a build directory into target_directory.

    A source that several configurations compile keeps its first entry. Returns the sorted
    source files.
    """
    with open(
        os.path.join(build_directory, "compile_commands.json"), encoding="utf-8"
    ) as database:
        entries = json.load(database)
    selected = {}
    for entry in entries:
        path = os.path.normpath(os.path.join(entry["directory"], entry["file"]))
        if path not in selected and is_source_path(root, path):
            selected[path] = {
                "directory": entry["directory"],
                "file": path,
                "arguments": compilation_arguments(entry),
            }
    with open(
        os.path.join(target_directory, "compile_commands.json"), "w", encoding="utf-8"
    ) as database:
        json.dump([selected[path] for path in sorted(selected)], database, indent=2)
    return sorted(selected)


CLANG_DIAGNOSTIC = re.compile(
    r"^(?P<path>[^\s:][^:]*):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): (?P<message>.*)$",
    re.MULTILINE,
)


def clang_findings(output: str) -> list[Finding]:
    """Returns the warnings and errors a clang tool prints, without notes and source excerpts."""
    return [
        Finding(
            os.path.normpath(match.group("path")),
            int(match.group("line")),
            f"{match.group('severity')}: {match.group('message')}",
        )
        for match in CLANG_DIAGNOSTIC.finditer(output)
    ]


RUFF_DIAGNOSTIC = re.compile(
    r"^(?P<path>[^\s:][^:]*):(?P<line>\d+):(?P<column>\d+): (?P<message>[A-Z]+\d+ .*)$",
    re.MULTILINE,
)


def ruff_findings(output: str) -> list[Finding]:
    """Returns the diagnostics of a ruff check in the concise output format."""
    return [
        Finding(
            os.path.abspath(match.group("path")),
            int(match.group("line")),
            match.group("message"),
        )
        for match in RUFF_DIAGNOSTIC.finditer(output)
    ]


def ruff_check(
    executable: str,
    rules: tuple[str, ...],
    ignored: tuple[str, ...],
    fix: bool,
    files: list[str],
) -> list[Finding]:
    """Returns the findings of a ruff check of the files. A failing ruff raises a ToolError."""
    [result] = run_commands([ruff_command(executable, rules, ignored, fix, files)], 1)
    if result.returncode not in (0, 1):
        raise ToolError(result.stderr.strip())
    return ruff_findings(result.stdout)


def ruff_command(
    executable: str,
    rules: tuple[str, ...],
    ignored: tuple[str, ...],
    fix: bool,
    files: list[str],
) -> list[str]:
    """Returns the ruff call that checks the files for the rules and applies the safe fixes on request."""
    command = [
        executable,
        "check",
        "--no-cache",
        "--output-format",
        "concise",
        "--select",
        ",".join(rules),
    ]
    if ignored:
        command += ["--ignore", ",".join(ignored)]
    if fix:
        command.append("--fix")
    return command + files


def report(findings: list[Finding], root: str, summary: str) -> int:
    """Prints the findings once each in path order, then the summary, and returns the exit status."""
    unique = sorted(set(findings))
    for finding in unique:
        print(finding.render(root))
    print(summary.format(len(unique)))
    return 1 if unique else 0
