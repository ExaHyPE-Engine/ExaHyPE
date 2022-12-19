# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

havejinja2=False
try:
  import jinja2
except ImportError:
  raise("Sympy is not available, some functionality might not be provided.")

import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.toolbox

from .Project import Project

print( "Peano 4 (C) www.peano-framework.org" )
