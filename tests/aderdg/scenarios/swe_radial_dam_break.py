# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import SWE_W_Bathymetry


class SWERadialDamBreak(Scenario):
    """
    Classic radial dam break SWE equations, with constant initial water height but
    a bump in the bathymetry in the centre.
    """

    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = False
    _dimensions = 2
    _equation = SWE_W_Bathymetry()
    _end_time = 0.5

    def __init__(self):
        return

    def initial_conditions(self):
        return """
  // The division halves the cell count per axis and rounds down.
  // With an odd cell count per axis the middle cell contains the centre of the domain.
  // The centre of the circle sits that many cells right of the left domain boundary.
  const int numberOfCells = DomainSize[0]/(2*h[0]);

  tarch::la::Vector<DIMENSIONS, double> damBreakCenter = {
    DomainOffset[0] + (numberOfCells * h[0]),
    0.0
  };

  Q.h  = 2;
  Q.hv = 0.0;
  Q.b  = (tarch::la::norm2(x - damBreakCenter) < 0.2 ? 0.5 : 0);
"""

    def boundary_conditions(self):
        return """
  Qoutside.h  = 2;
  Qoutside.hv = 0.0;
  Qoutside.b  = 0;
"""
