# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

try:
  import jinja2
except ImportError:
  raise Exception("Jinja2 is not available, cannot generate glue code.")

havevtk=False
try:
  import vtk
  havevtk=True
except ImportError:
  print("VTK is not available, not loading peano4.visualisation.")

haveparaview=False
try:
  import paraview
  haveparaview=True
except ImportError:
  print("ParaView is not available, not loading peano4.visualisation.")

if havevtk and haveparaview:
  import peano4.visualisation

import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.toolbox

from .Project import Project

print("Peano 4 (C) www.peano-framework.org")
