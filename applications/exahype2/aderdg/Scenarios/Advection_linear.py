from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Advection

class Advection_linear(Scenario):

    """
    Very simple scenario in which the initial value of x is shifted
    in each spatial dimension.
    """

    _plot_dt     = 0.1
    _periodic_bc = False

    def __init__(
        self,
        dimensions=2
    ):
        self._dimensions  = dimensions
        self._equation    = Advection(dimensions=dimensions)

    def initial_conditions(self):
        return """
  Q[0] = x[0];
  Q[1] = x[1];
#if Dimensions==3
  Q[2] = x[2];
#endif
"""

    def boundary_conditions(self):
        return """
  Qoutside[0] = faceCentre[0] - t;
  Qoutside[1] = faceCentre[1] - t;
#if Dimensions==3
  Qoutside[2] = faceCentre[2] - t;
#endif
"""