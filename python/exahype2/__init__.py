# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import exahype2.solvers.fv
import exahype2.solvers.rkfd
import exahype2.solvers.elliptic

havenumpy=False
try:
    import numpy
    import mpmath
    havenumpy=True
except ImportError:
    print("Numpy/mpmath is not available, not loading exahype2.solvers.rkdg")
    print("Numpy/mpmath is not available, not loading exahype2.solvers.aderdg")

if havenumpy:
    import exahype2.solvers.rkdg
    #import exahype2.solvers.aderdg

havesympy=False
try:
    import sympy
    havesympy=True
except ImportError:
    print("Sympy is not available, not loading exahype2.symhype")

if havesympy:
    import exahype2.symhype

import exahype2.tracer
import exahype2.grid
import exahype2.gpu

from .ExaHyPEMain import ExaHyPEMain
from .Project     import Project

print( "ExaHyPE 2 (C) www.peano-framework.org" )
