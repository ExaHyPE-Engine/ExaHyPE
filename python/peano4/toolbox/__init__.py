# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


try:
  from .CreateRegularGrid                      import CreateRegularGrid
  from .PlotGridInPeanoBlockFormat             import PlotGridInPeanoBlockFormat
  from .PlotVertexDataInPeanoBlockFormat       import PlotVertexDataInPeanoBlockFormat
  from .PlotCellDataInPeanoBlockFormat         import PlotCellDataInPeanoBlockFormat
  
  from .TaskNumbersOnMesh                      import AssignTaskNumbersToMesh
  from .TaskNumbersOnMesh                      import ClearTaskNumbersOnMesh
  import TaskNumbersOnMesh                     # for utility functions therein
except ImportError:
  print("Numpy/mpmath is not available, not loading rkdg")
  print("Numpy/mpmath is not available, not loading aderdg")

