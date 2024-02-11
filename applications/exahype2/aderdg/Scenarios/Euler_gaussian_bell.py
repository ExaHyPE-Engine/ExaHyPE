from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Euler

class Euler_gaussian_bell(Scenario):

    """
    Scenario reproduced from Ioratti, Dumbser & Loubère, https://doi.org/10.1007/s10915-020-01209-w (p. 44)
    """

    _plot_dt     = 0.0
    _offset      = -.5
    _domain_size = 1.0
    _periodic_bc = True

    def __init__(
        self,
        iterations=2
    ):
        self._dimensions  = 2
        self._end_time    = iterations
        self._equation    = Euler(dimensions=2, gamma=1.4)

    def initial_conditions(self):
        return """
  Q[0] = 0.02*(1.0+std::exp(-0.5*tarch::la::norm2(x)*tarch::la::norm2(x)/0.01));

  Q[1] = 1.0*Q[0];
  Q[2] = 1.0*Q[0];

  constexpr double gamma = 1.4;
  constexpr double p = 1.0;

  //Should lead to a constant p over the domain given the initial conditions
  Q[3] = p/(gamma-1.0) + 0.5*(Q[1]*Q[1]+Q[2]*Q[2]) / Q[0];
"""

    def analytical_solution(self):
        return """

  /*
    The initial condition is transported without deformation in a periodic
    domain [-.5, +.5], i.e. the value at a given point is the value at the
    position x - v*t but accounting for periodicity.
    Therefore we find the point x - v*t, and then shift this by instances
    of 1.0 until we find the point within the domain. 
  */
  tarch::la::Vector<Dimensions,double> pos = { (x[0] - t) + (int)(t + .5 - x[0]), 
                                               (x[1] - t) + (int)(t + .5 - x[1]) };

  sol[0] = 0.02*(1.0+std::exp(-0.5*tarch::la::norm2(x)*tarch::la::norm2(x)/0.01));

  sol[1] = 1.0*sol[0];
  sol[2] = 1.0*sol[0];
  sol[3] = 0.;

  constexpr double gamma = 1.4;
  constexpr double p = 1.0;

  //Should lead to a constant p over the domain given the initial conditions
  sol[3] = p/(gamma-1.0) + 0.5*(sol[1]*sol[1]+sol[2]*sol[2]) / sol[0];
"""