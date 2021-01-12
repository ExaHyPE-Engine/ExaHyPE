# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.fv

havenumpy=False
try:
    import numpy
    havenumpy=True
except ImportError:
    print("Numpy is not available, not loading dg")
    print("Numpy is not available, not loading aderdg")

if havenumpy:
    import exahype2.solvers.aderdg
    import exahype2.solvers.dg
    
import exahype2.grid
import exahype2.gpu


from .ExaHyPEMain import ExaHyPEMain
from .Project     import Project


print( "ExaHyPE 2 (C) www.peano-framework.org" )
