# This file is part of the SWIFT 2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .Project import Project
from .PETScMain import PETScMain

import petsc.solvers

#import matrix generator base class
import petsc.matrixgenerators

print( "PETSc backend" )

