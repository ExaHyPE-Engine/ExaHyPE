from .Scenario import Scenario

import os, sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import SWE_W_Bathimetry

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
"""
}

class SWE_resting_lake(Scenario):
    
    """
    Resting lake scenario for the shallow water equations.
    The real water height H as defined by the sum of the water column h and
    the bathimetry b is constant over the entire domain, meaning that there
    should be no changes on the entire domain, but because we use the sum of
    the derivatives of h and b (h' + b') instead of the derivative of the sum
    (h + b)' some rounding errors will creep in, which causes unphysical
    waves to appear.
    As such this scenario is nice for testing how large these unphysical waves
    are for a given algorithm, and how stable the algorithm is, i.e. does it
    dampen out these waves or do they oscillate out of control.
    """

    _plot_dt     = 0.0
    _offset      = -.5
    _domain_size = 1.0
    _periodic_bc = True
    _dimensions  = 2
    _equation    = SWE_W_Bathimetry()
    _end_time    = 1.0

    def __init__(
        self,
        initial_conditions = "sinusoidal",
    ):
        self._init    = _initial_conditions[initial_conditions]

    def initial_conditions(self):
        return self._init

    def analytical_solution(self):
        return """
  double Q[4];\n""" + self._init + """

  sol[0] = Q[0];
  sol[1] = Q[1];
  sol[2] = Q[2];
  sol[3] = Q[3];
  """