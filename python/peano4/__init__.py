# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

try:
  import jinja2
except ImportError:
  raise Exception("Jinja2 is not available, cannot generate glue code.")

havepvtk=False
try:
  import vtk
  import paraview
  havepvtk=True
except ImportError:
  print("VTK and ParaView are not available, not loading peano4.visualisation.")

if havepvtk:
  import peano4.visualisation

import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.toolbox

from .Project import Project
from .Project import merge

print("Peano 4 (C) www.peano-framework.org")
