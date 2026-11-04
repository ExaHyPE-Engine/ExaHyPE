# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import ast
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
from postprocessing.source_tree import ruff_check
from postprocessing.source_tree import run_commands
from postprocessing.source_tree import source_files

# Import order and grouping, unused, repeated and star imports, several modules in one
# statement, imports below code and imports through a parent package.
IMPORT_RULES = ("I", "F401", "F403", "F405", "F811", "E401", "E402", "TID252")

MISSING_MODULE = re.compile(
    r"^ModuleNotFoundError: No module named '(?P<module>[^']+)'$"
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Checks the imports of every Python file of the repository: Order, unused and "
        "stale imports, and whether every module of a package imports in a fresh interpreter."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Sorts the import blocks and removes the unused imports ruff can remove safely",
    )
    return parser


def attribute_chains(tree: ast.AST) -> set[str]:
    """Returns the dotted names a module reads, each at its full length."""
    inner = {
        id(node.value) for node in ast.walk(tree) if isinstance(node, ast.Attribute)
    }
    chains = set()
    for node in ast.walk(tree):
        if not isinstance(node, (ast.Attribute, ast.Name)) or id(node) in inner:
            continue
        parts = []
        while isinstance(node, ast.Attribute):
            parts.append(node.attr)
            node = node.value
        if isinstance(node, ast.Name):
            parts.append(node.id)
            chains.add(".".join(reversed(parts)))
    return chains


def stale_submodule_imports(path: str, text: str) -> list[Finding]:
    """Returns every ``import a.b`` whose module ``a.b`` the file never reads.

    Other modules import the package ``a`` as well. A checker that tracks only ``a`` misses
    such an import.
    """
    try:
        tree = ast.parse(text, path)
    except SyntaxError as error:
        return [Finding(path, error.lineno or 0, f"cannot parse the file: {error.msg}")]
    chains = attribute_chains(tree)
    findings = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Import):
            continue
        for alias in node.names:
            if alias.asname is not None or "." not in alias.name:
                continue
            if not any(
                chain == alias.name or chain.startswith(alias.name + ".")
                for chain in chains
            ):
                findings.append(
                    Finding(path, node.lineno, f"import {alias.name} is unused")
                )
    return findings


def top_level_packages(root: str, files: list[str]) -> list[str]:
    """Returns the sorted package directories whose parent directory is no package."""
    packages = {
        os.path.dirname(path)
        for path in files
        if os.path.basename(path) == "__init__.py"
    }
    return sorted(
        directory
        for directory in packages
        if directory != os.path.abspath(root)
        and not os.path.isfile(os.path.join(os.path.dirname(directory), "__init__.py"))
    )


def package_modules(package: str, files: list[str]) -> list[tuple[str, str]]:
    """Returns the dotted module names and the files of a package, the package itself first.

    A file counts only when every directory from the package down to the file contains an
    ``__init__.py``. No script of a plain subdirectory then runs.
    """
    initialised = {
        os.path.dirname(path)
        for path in files
        if os.path.basename(path) == "__init__.py"
    }
    parent = os.path.dirname(package)
    modules = []
    for path in files:
        directory = os.path.dirname(path)
        if os.path.commonpath([directory, package]) != package:
            continue
        relative = os.path.relpath(directory, package)
        steps = [] if relative == os.curdir else relative.split(os.sep)
        if not all(
            os.path.join(package, *steps[: depth + 1]) in initialised
            for depth in range(len(steps))
        ):
            continue
        name = os.path.relpath(path, parent)[: -len(".py")].replace(os.sep, ".")
        if name.endswith(".__init__"):
            name = name[: -len(".__init__")]
        modules.append((name, path))
    return sorted(modules)


def import_findings(
    output: str, module: str, path: str, repository_modules: set[str]
) -> list[Finding]:
    """Returns the finding of a failed import, or none when a module outside the repository is missing."""
    lines = output.strip().splitlines()
    if not lines:
        return [Finding(path, 0, f"import {module} fails")]
    missing = MISSING_MODULE.match(lines[-1])
    if (
        missing is not None
        and missing.group("module").split(".")[0] not in repository_modules
    ):
        return []
    return [Finding(path, 0, f"import {module} fails with {lines[-1]}")]


def fresh_interpreter_findings(root: str, files: list[str], jobs: int) -> list[Finding]:
    """Imports every module of every package on its own and returns the modules whose import fails."""
    packages = top_level_packages(root, files)
    repository_modules = {os.path.basename(package) for package in packages}
    findings = []
    for package in packages:
        # The own package comes first. A package of the same name elsewhere then cannot hide it.
        search_path = [os.path.dirname(package)] + [
            os.path.dirname(other)
            for other in packages
            if os.path.basename(other) != os.path.basename(package)
        ]
        search_path += [
            entry
            for entry in os.environ.get("PYTHONPATH", "").split(os.pathsep)
            if entry
        ]
        environment = dict(
            os.environ,
            PYTHONPATH=os.pathsep.join(dict.fromkeys(search_path)),
            PYTHONDONTWRITEBYTECODE="1",
        )
        modules = package_modules(package, files)
        results = run_commands(
            [[sys.executable, "-c", f"import {module}"] for module, _ in modules],
            jobs,
            environment,
        )
        for (module, path), result in zip(modules, results):
            if result.returncode != 0:
                findings.extend(
                    import_findings(result.stderr, module, path, repository_modules)
                )
    return findings


def main() -> None:
    args = build_parser().parse_args()
    try:
        root = resolve_root(args.root)
        files = source_files(root, PYTHON_SUFFIXES)
        findings = ruff_check(require_tool("ruff"), IMPORT_RULES, (), args.fix, files)
    except (FileNotFoundError, ToolError) as error:
        print(error)
        sys.exit(2)
    for path in files:
        with open(path, encoding="utf-8", errors="replace") as source:
            findings.extend(stale_submodule_imports(path, source.read()))
    findings.extend(fresh_interpreter_findings(root, files, args.jobs))
    sys.exit(
        report(
            findings,
            root,
            "{} import findings in " + str(len(files)) + " Python files.",
        )
    )


if __name__ == "__main__":
    main()
