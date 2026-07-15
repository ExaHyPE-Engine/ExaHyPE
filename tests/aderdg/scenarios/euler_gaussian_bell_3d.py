# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Euler


class EulerGaussianBell3D(Scenario):
    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = True

    def __init__(self):
        self._dimensions = 3
        self._end_time = 0.1
        self._equation = Euler(dimensions=3, gamma=1.4)

    def initial_conditions(self):
        return """
  Q[Shortcuts::rho] = 0.02*(1+std::exp(-0.5*tarch::la::norm2(x)*tarch::la::norm2(x)/0.01));

  Q[Shortcuts::v+0] = Q[Shortcuts::rho];
  Q[Shortcuts::v+1] = Q[Shortcuts::rho];
  Q[Shortcuts::v+2] = Q[Shortcuts::rho];

  constexpr auto gamma = 1.4;
  constexpr auto p = 1;

  //Should lead to a constant p over the domain given the initial conditions
  Q[Shortcuts::E] = p/(gamma-1) + 0.5*(Q[Shortcuts::v+0]*Q[Shortcuts::v+0]+Q[Shortcuts::v+1]*Q[Shortcuts::v+1]+Q[Shortcuts::v+2]*Q[Shortcuts::v+2]) / Q[Shortcuts::rho];
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
  tarch::la::Vector<DIMENSIONS,double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                               (x[1] - t) + (int)(t + .5 - x[1]),
                                               (x[2] - t) + (int)(t + .5 - x[2]) };

  solution[Shortcuts::rho] = 0.02*(1+std::exp(-0.5*tarch::la::norm2(pos)*tarch::la::norm2(pos)/0.01));

  solution[Shortcuts::v+0] = solution[Shortcuts::rho];
  solution[Shortcuts::v+1] = solution[Shortcuts::rho];
  solution[Shortcuts::v+2] = solution[Shortcuts::rho];

  constexpr auto gamma = 1.4;
  constexpr auto p = 1;

  //Should lead to a constant p over the domain given the initial conditions
  solution[Shortcuts::E] = p/(gamma-1) + 0.5*(solution[Shortcuts::v+0]*solution[Shortcuts::v+0]+solution[Shortcuts::v+1]*solution[Shortcuts::v+1]+solution[Shortcuts::v+2]*solution[Shortcuts::v+2]) / solution[Shortcuts::rho];
"""
