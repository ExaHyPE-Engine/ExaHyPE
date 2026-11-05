# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import importlib

_LAZY_MODULES = {
    "codegen": "exahype2.codegen",
    "dg": "exahype2.dg",
    "fv": "exahype2.fv",
    "particles": "exahype2.particles",
}

_LAZY_ATTRIBUTES = {
    "log": (".log", "log"),
    "Main": (".main", "Main"),
    "Precision": (".precision", "Precision"),
    "Project": (".project", "Project"),
    "SymmetricMatrix": (".state", "SymmetricMatrix"),
    "TimeStepping": (".time_stepping", "TimeStepping"),
}

__all__ = list(_LAZY_MODULES.keys()) + list(_LAZY_ATTRIBUTES.keys())


def __getattr__(name):
    if name in _LAZY_MODULES:
        value = importlib.import_module(_LAZY_MODULES[name])
        globals()[name] = value
        return value

    if name in _LAZY_ATTRIBUTES:
        module_name, attribute_name = _LAZY_ATTRIBUTES[name]
        module = importlib.import_module(module_name, __name__)
        value = getattr(module, attribute_name)
        globals()[name] = value
        return value

    raise AttributeError(f"The module {__name__!r} cannot find the attribute {name!r}.")
