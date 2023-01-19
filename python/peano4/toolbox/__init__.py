# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


try:
  from .CreateRegularGrid                      import CreateRegularGrid
  from .PlotGridInPeanoBlockFormat             import PlotGridInPeanoBlockFormat
  from .PlotVertexDataInPeanoBlockFormat       import PlotVertexDataInPeanoBlockFormat
  from .PlotCellDataInPeanoBlockFormat         import PlotCellDataInPeanoBlockFormat
except ImportError:
  print("Numpy/mpmath is not available, not loading rkdg")
  print("Numpy/mpmath is not available, not loading aderdg")

