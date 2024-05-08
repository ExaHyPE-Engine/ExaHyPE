# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


print("Peano 4 (C) www.peano-framework.org")

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

try:
  import jinja2
except ImportError:
  print("WARNING: Jinja2 is not available, cannot generate glue code. Most Peano 4 features will be unavailable")

try:
  import peano4.datamodel
  import peano4.solversteps
  import peano4.output
  import peano4.toolbox
  
  from .Project import Project
  # from .Project import merge
except ImportError as error:
  print( "Peano core project files cannot be loaded: {}".format(error) )


