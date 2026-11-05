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

    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = True

    def __init__(self, iterations=2):
        self._dimensions = 2
        self._end_time = iterations
        self._equation = Euler(dimensions=2, gamma=1.4)

    def initial_conditions(self):
        return """
  Q.rho = 0.02*(1+std::exp(-0.5*tarch::la::norm2(x)*tarch::la::norm2(x)/0.01));
  Q.v   = Q.rho;

  constexpr auto gamma = 1.4;
  constexpr auto p = 1;

  // The energy keeps the pressure constant over the domain.
  Q.E = p/(gamma-1) + 0.5*(Q.v(0)*Q.v(0)+Q.v(1)*Q.v(1)) / Q.rho;
"""

    def analytical_solution(self):
        return """
  // The initial condition moves unchanged with velocity 1 along every axis. The solution at x
  // is the initial value at x - t, wrapped into the periodic domain.
  tarch::la::Vector<DIMENSIONS,double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                               (x[1] - t) + (int)(t + .5 - x[1]) };

  solution.rho = 0.02*(1+std::exp(-0.5*tarch::la::norm2(pos)*tarch::la::norm2(pos)/0.01));
  solution.v   = solution.rho;

  constexpr auto gamma = 1.4;
  constexpr auto p = 1;

  // The energy keeps the pressure constant over the domain.
  solution.E = p/(gamma-1) + 0.5*(solution.v(0)*solution.v(0)+solution.v(1)*solution.v(1)) / solution.rho;
"""
