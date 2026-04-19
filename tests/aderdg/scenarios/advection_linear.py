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
  Q[Shortcuts::v+0] = x[0];
  Q[Shortcuts::v+1] = x[1];
#if DIMENSIONS==3
  Q[Shortcuts::v+2] = x[2];
#endif
"""

    def boundary_conditions(self):
        return """
  Qoutside[Shortcuts::v+0] = x[0] - t;
  Qoutside[Shortcuts::v+1] = x[1] - t;
#if DIMENSIONS==3
  Qoutside[Shortcuts::v+2] = x[2] - t;
#endif
"""
