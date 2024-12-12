# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import SWE_W_Bathymetry

_initial_conditions = {
    "bilinear": """
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = 1.0 - std::abs(x[0]) - std::abs(x[1]);
    Q[0] = 2.0 - Q[3];
""",
    "sinusoidal": """
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = sin( 2*M_PI * (x[0]+x[1]) );
    Q[0] = 2.0 - Q[3];
""",
    "constant": """
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = 1.0;
    Q[0] = 2.0 - Q[3];
""",
}


class SWERestingLake(Scenario):
    """
    Resting lake scenario for the shallow water equations.
    The real water height H as defined by the sum of the water column h and
    the bathymetry b is constant over the entire domain, meaning that there
    should be no changes on the entire domain, but because we use the sum of
    the derivatives of h and b (h' + b') instead of the derivative of the sum
    (h + b)' some rounding errors will creep in, which causes unphysical
    waves to appear.
    As such this scenario is nice for testing how large these unphysical waves
    are for a given algorithm, and how stable the algorithm is, i.e. does it
    dampen out these waves or do they oscillate out of control.
    """

    _plot_dt = 0.0
    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = True
    _dimensions = 2
    _equation = SWE_W_Bathymetry()
    _end_time = 1.0

    def __init__(
        self,
        initial_conditions="sinusoidal",
    ):
        self._init = _initial_conditions[initial_conditions]

    def initial_conditions(self):
        return self._init

    def analytical_solution(self):
        return """
  double _Q[4];
  initialCondition(
      _Q,
      x,
      tarch::la::Vector<Dimensions, double>(),
      true
  );

  solution[0] = _Q[0];
  solution[1] = _Q[1];
  solution[2] = _Q[2];
  solution[3] = _Q[3];
  """
