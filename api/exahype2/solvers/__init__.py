# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import importlib

from .PDETerms import PDETerms
from .FixedTimeSteppingCodeSnippets import FixedTimeSteppingCodeSnippets
from .AdaptiveTimeSteppingCodeSnippets import AdaptiveTimeSteppingCodeSnippets
from .LagrangeBasisWithDiagonalMassMatrix import GaussLegendreBasis, GaussLobattoBasis
from .MathUtils import MathUtils
from .ErrorMeasurement import ErrorMeasurement

_LAZY_MODULES = {
    "fv": "exahype2.solvers.fv",
    "rkdg": "exahype2.solvers.rkdg",
    "aderdg": "exahype2.solvers.aderdg",
    "limiting": "exahype2.solvers.limiting",
}

__all__ = list(_LAZY_MODULES.keys()) + [
    "PDETerms",
    "FixedTimeSteppingCodeSnippets",
    "AdaptiveTimeSteppingCodeSnippets",
    "GaussLegendreBasis",
    "GaussLobattoBasis",
    "MathUtils",
    "ErrorMeasurement",
]


def __getattr__(name):
    if name in _LAZY_MODULES:
        value = importlib.import_module(_LAZY_MODULES[name])
        globals()[name] = value
        return value

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
