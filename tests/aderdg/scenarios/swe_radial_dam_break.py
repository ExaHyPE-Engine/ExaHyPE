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
  // The number of cells will be odd, this computes half of the total -1
  // e.g. 27 cells -> 13, 81 cells -> 40
  // We then place the circle so that it's center is that many cells from
  // the left domain boundary.
  const int numberOfCells = DomainSize[0]/(2*h[0]);

  tarch::la::Vector<DIMENSIONS, double> damBreakCenter = {
    DomainOffset[0] + (numberOfCells * h[0]),
    0.0
  };

  Q[Shortcuts::h]   = 2; // h
  Q[Shortcuts::v+0] = 0; // v_x
  Q[Shortcuts::v+1] = 0; // v_y
  Q[Shortcuts::b]   = (tarch::la::norm2(x - damBreakCenter) < 0.2 ? 0.5 : 0); // b
"""

    def boundary_conditions(self):
        return """
  Qoutside[Shortcuts::h]    = 2; // h
  Qoutside[Shortcuts::v+0]  = 0; // v_x
  Qoutside[Shortcuts::v+1]  = 0; // v_y
  Qoutside[Shortcuts::b]    = 0; // b
"""
