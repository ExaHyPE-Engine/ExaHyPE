# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import importlib

from .Logger import log
from .Project import Project
from .PeanoMain import PeanoMain

_LAZY_MODULES = {
    "datamodel": "peano4.datamodel",
    "solversteps": "peano4.solversteps",
    "output": "peano4.output",
    "blockstructured": "peano4.blockstructured",
    "particles": "peano4.particles",
    "plotter": "peano4.plotter",
}

__all__ = list(_LAZY_MODULES.keys()) + ["log", "Project", "PeanoMain"]


def __getattr__(name):
    if name in _LAZY_MODULES:
        value = importlib.import_module(_LAZY_MODULES[name])
        globals()[name] = value
        return value

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
