# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import ast
import os
import sys

API = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..")
PACKAGE = os.path.join(API, "exahype2")

LAYERS = {
    "exahype2.log": 0,
    "exahype2.codegen": 0,
    "exahype2.precision": 0,
    "exahype2.state": 3,
    "exahype2.pde_terms": 3,
    "exahype2.particles": 3,
    "exahype2.time_stepping": 3,
    "exahype2.solver": 3,
    "exahype2.fv": 4,
    "exahype2.dg": 4,
    "exahype2.main": 5,
    "exahype2.project": 5,
    "exahype2": 6,
}


def layer_of(module):
    """Returns the layer of a module name, the most specific table entry that prefixes it."""
    candidate = module
    while candidate:
        if candidate in LAYERS:
            return LAYERS[candidate]
        candidate = candidate.rpartition(".")[0]
    raise KeyError(module)


def module_name(path):
    relative = os.path.relpath(path, API)[: -len(".py")]
    if relative.endswith("/__init__"):
        relative = relative[: -len("/__init__")]
    return relative.replace(os.sep, ".")


def imported_modules(path, module):
    """Returns the exahype2 modules a file imports, relative imports resolved."""
    package = module if path.endswith("__init__.py") else module.rpartition(".")[0]
    with open(path) as source:
        tree = ast.parse(source.read(), path)
    result = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            result += [
                alias.name for alias in node.names if alias.name.startswith("exahype2")
            ]
        elif isinstance(node, ast.ImportFrom):
            if node.level == 0:
                base = node.module
            else:
                base = ".".join(
                    package.split(".")[: len(package.split(".")) - node.level + 1]
                )
                if node.module:
                    base += "." + node.module
            if not base.startswith("exahype2"):
                continue
            if node.module is None:
                result += [base + "." + alias.name for alias in node.names]
            else:
                result.append(base)
    return result


def main():
    violations = []
    for directory, _, files in os.walk(PACKAGE):
        for name in files:
            if not name.endswith(".py"):
                continue
            path = os.path.join(directory, name)
            module = module_name(path)
            if module == "exahype2":
                continue
            for imported in imported_modules(path, module):
                if layer_of(imported) > layer_of(module):
                    violations.append("{} imports {}".format(module, imported))
    for violation in sorted(violations):
        print(violation)
    print("{} violations".format(len(violations)))
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
