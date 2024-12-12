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

    _plot_dt = 0.05
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
  Q[0] = 2.0; // h
  Q[1] = 0.0; // v_x
  Q[2] = 0.0; // v_y
  Q[3] = (tarch::la::norm2(x) < 0.2 ? 0.5 : 0.0); // b
"""

    def boundary_conditions(self):
        return """
  Qoutside[0] = 2.0; // h
  Qoutside[1] = 0.0; // v_x
  Qoutside[2] = 0.0; // v_y
  Qoutside[3] = 0.0; // b
"""
