# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Euler


class EulerGaussianBell(Scenario):
    """
    Scenario reproduced from Ioratti, Dumbser & Loubère, https://doi.org/10.1007/s10915-020-01209-w (p. 44)
    """

    _plot_dt = 0.0
    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = True

    def __init__(self, iterations=2):
        self._dimensions = 2
        self._end_time = iterations
        self._equation = Euler(dimensions=2, gamma=1.4)

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

  solution[0] = 0.02*(1.0+std::exp(-0.5*tarch::la::norm2(pos)*tarch::la::norm2(pos)/0.01));

  solution[1] = 1.0*solution[0];
  solution[2] = 1.0*solution[0];
  solution[3] = 0.;

  constexpr double gamma = 1.4;
  constexpr double p = 1.0;

  //Should lead to a constant p over the domain given the initial conditions
  solution[3] = p/(gamma-1.0) + 0.5*(solution[1]*solution[1]+solution[2]*solution[2]) / solution[0];
"""
