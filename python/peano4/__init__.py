import peano4.datamodel
import peano4.solversteps
import peano4.output

try:
  import peano4.toolbox
except ImportError:
  print("Some required libraries (likely jinja2) are not available. Some functionality might note be provided.")

from .Project import Project

print( "Peano 4 (C) www.peano-framework.org" )

