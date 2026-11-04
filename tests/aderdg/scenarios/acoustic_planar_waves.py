# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys
from math import sqrt

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Acoustic


class AcousticPlanarWaves(Scenario):
    """
    Scenario reproduced from Dumbser & Käser, https://doi.org/10.1111/j.1365-246X.2006.03120.x (Chapter 5, p. 328)
    """

    _offset = -1.0
    _domain_size = 2.0
    _periodic_bc = True

    def __init__(self, dimensions, iterations=2):
        self._dimensions = dimensions
        self._end_time = iterations * sqrt(dimensions)
        self._K0 = 4
        self._rho = 1
        self._equation = Acoustic(dimensions, rho=self._rho, K0=self._K0)

    def initial_conditions(self):
        return (
            """
  // simple translation in positive diagonal direction
  // tarch::la::sumInOrderOfMagnitude() returns the same bits for every order of the coordinates.
#if DIMENSIONS==3
  const double sumOfCoordinates = tarch::la::sumInOrderOfMagnitude(x[0], x[1], x[2]);
#else
  const double sumOfCoordinates = x[0] + x[1];
#endif
  const double val = cos( - std::numbers::pi*sumOfCoordinates);

  Q[Shortcuts::v+0] = val;
  Q[Shortcuts::v+1] = val;
#if DIMENSIONS==3
  Q[Shortcuts::v+2] = val;
#endif

  constexpr auto K0  = """
            + str(self._K0)
            + """;
  constexpr auto rho = """
            + str(self._rho)
            + """;

  // These are defined by the eigenvector of the plane wave operator
#if DIMENSIONS==3
  const auto kr = K0*std::sqrt(rho/(3*K0)) + 2*std::sqrt(K0*rho/3);
#else
  const auto kr = std::sqrt(2*K0*rho);
#endif

  Q[Shortcuts::p] = kr*val;
"""
        )

    def analytical_solution(self):
        return (
            """
  const double w = 2*std::sqrt(DIMENSIONS)*M_PI;
#if DIMENSIONS==3
  const double sumOfCoordinates = tarch::la::sumInOrderOfMagnitude(x[0], x[1], x[2]);
#else
  const double sumOfCoordinates = x[0] + x[1];
#endif
  const double val = cos( w*t - std::numbers::pi*sumOfCoordinates);

  solution[Shortcuts::v+0] = val;
  solution[Shortcuts::v+1] = val;

  constexpr auto K0  = """
            + str(self._K0)
            + """;
  constexpr auto rho = """
            + str(self._rho)
            + """;

#if DIMENSIONS==3
  solution[Shortcuts::v+2] = val;
  const auto kr = K0*std::sqrt(rho/(3*K0)) + 2*std::sqrt(K0*rho/3);
#else
  const auto kr = std::sqrt(2*K0*rho);
#endif

  solution[Shortcuts::p] = kr*val;
"""
        )
