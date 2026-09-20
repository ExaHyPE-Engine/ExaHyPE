# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import importlib

from .basis import GaussLegendreBasis as GaussLegendre
from .basis import GaussLobattoBasis as GaussLobatto

_LAZY_MODULES = {
    "ader": "exahype2.dg.ader",
    "limiting": "exahype2.dg.limiting",
    "rk": "exahype2.dg.rk",
}

__all__ = list(_LAZY_MODULES.keys()) + ["GaussLegendre", "GaussLobatto"]


def __getattr__(name):
    if name in _LAZY_MODULES:
        value = importlib.import_module(_LAZY_MODULES[name])
        globals()[name] = value
        return value

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
