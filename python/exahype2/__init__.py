# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import exahype2.tracer
import exahype2.errorMeasurement
import exahype2.grid

import exahype2.solvers.fv
import exahype2.solvers.rkfd
import exahype2.solvers.elliptic

havenumpy = False
try:
    import numpy
    havenumpy = True
except ImportError:
    print("Numpy is not available, not loading exahype2.symhype.")
    print("Numpy is not available, not loading exahype2.solvers.rkdg.")
    print("Numpy is not available, not loading exahype2.solvers.aderdg.")

havempmath = False
try:
    import mpmath
    havempmath = True
except ImportError:
    print("Mpmath is not available, not loading exahype2.solvers.rkdg.")
    print("Mpmath is not available, not loading exahype2.solvers.aderdg.")

if havenumpy and havempmath:
    import exahype2.solvers.rkdg
    import exahype2.solvers.aderdg

havesympy = False
try:
    import sympy
    havesympy = True
except ImportError:
    print("Sympy is not available, not loading exahype2.symhype.")

if havenumpy and havesympy:
    import exahype2.symhype

havematplotlib = False
try:
    import matplotlib
    havematplotlib = True
except ImportError:
    print("Matplotlib is not available, not loading exahype2.postprocessing.")

if havematplotlib:
    import exahype2.postprocessing

from .ExaHyPEMain import ExaHyPEMain
from .Project import Project

print("ExaHyPE 2 (C) www.peano-framework.org")
