# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Advection


class AdvectionLinear(Scenario):
    """
    Very simple scenario in which the initial value of x is shifted
    in each spatial dimension.
    """

    _periodic_bc = False

    def __init__(self, dimensions=2):
        self._dimensions = dimensions
        self._equation = Advection(dimensions=dimensions)

    def initial_conditions(self):
        return """
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    Q.v(axis) = x[axis];
  }
"""

    def boundary_conditions(self):
        return """
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    Qoutside.v(axis) = x[axis] - t;
  }
"""
