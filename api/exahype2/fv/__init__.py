# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import importlib

from .scheme import Fallback
from .scheme import Godunov
from .scheme import Limiter
from .scheme import MusclHancock
from .scheme import PolynomialLimiter
from .scheme import Weighting
from .scheme import Weno

_LAZY_MODULES = {
    "ader": "exahype2.fv.ader",
    "singlestep": "exahype2.fv.singlestep",
}

__all__ = list(_LAZY_MODULES.keys()) + [
    "Fallback",
    "Godunov",
    "Limiter",
    "MusclHancock",
    "PolynomialLimiter",
    "Weighting",
    "Weno",
]


def __getattr__(name):
    if name in _LAZY_MODULES:
        value = importlib.import_module(_LAZY_MODULES[name])
        globals()[name] = value
        return value

    raise AttributeError(f"The module {__name__!r} cannot find the attribute {name!r}.")
