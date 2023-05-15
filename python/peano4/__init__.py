# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

havejinja2=False
try:
  import jinja2
except ImportError:
  raise("Jinja2 is not available, some functionality might not be provided.")

havepvtk=False
try:
  import vtk
  import paraview
  havepvtk=True
except ImportError:
  print("VTK and ParaView is not available, not loading peano4.visualisation")

if havepvtk:
  import peano4.visualisation

import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.toolbox

from .Project import Project

print( "Peano 4 (C) www.peano-framework.org" )
