# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Elastic


class ElasticPlanarWaves(Scenario):
    """
    Scenario reproduced from Dumbser & Käser, https://doi.org/10.1111/j.1365-246X.2006.03120.x (Chapter 5, p. 328)
    """

    _offset = -1.0
    _domain_size = 2.0
    _periodic_bc = True

    def __init__(self, dimensions, iterations=2):
        self._dimensions = dimensions
        self._end_time = iterations
        self._equation = Elastic(dimensions)

    def initial_conditions(self):
        return """
  // Lamé parameters
  constexpr auto mu     = 1;
  constexpr auto lambda = 2;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 2;
  constexpr auto cs   = 1;

  Q.rho = rho;
  Q.cp  = cp;
  Q.cs  = cs;

  const double val_x = cos( -std::numbers::pi*x[0]);
  const double val_y = cos( -std::numbers::pi*x[1]);
""" + (
            """
  const double val_z = cos( -std::numbers::pi*x[2]);

  Q.sigma       = {};
  Q.sigma(0, 0) = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;
  Q.sigma(1, 1) = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;
  Q.sigma(2, 2) = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);

  Q.v(0) = val_x*cp;
  Q.v(1) = val_y*cp;
  Q.v(2) = val_z*cp;
"""
            if self._dimensions == 3
            else """
  Q.sigma       = {};
  Q.sigma(0, 0) = val_x*(lambda+2*mu)  + val_y*lambda;
  Q.sigma(1, 1) = val_x*lambda         + val_y*(lambda+2*mu);

  Q.v(0) = val_x*cp;
  Q.v(1) = val_y*cp;
"""
        )

    def analytical_solution(self):
        return """
  // Lamé parameters
  constexpr auto mu     = 1;
  constexpr auto lambda = 2;

  // Constant auxiliary parameters
  constexpr auto cp   = 2;

  constexpr double w = -2*M_PI;

  const double val_x = cos( w*t -std::numbers::pi*x[0]);
  const double val_y = cos( w*t -std::numbers::pi*x[1]);
""" + (
            """
  const double val_z = cos( -std::numbers::pi*x[2]);

  solution.sigma       = {};
  solution.sigma(0, 0) = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;
  solution.sigma(1, 1) = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;
  solution.sigma(2, 2) = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);

  solution.v(0) = val_x*cp;
  solution.v(1) = val_y*cp;
  solution.v(2) = val_z*cp;
"""
            if self._dimensions == 3
            else """
  solution.sigma       = {};
  solution.sigma(0, 0) = val_x*(lambda+2*mu)  + val_y*lambda;
  solution.sigma(1, 1) = val_x*lambda         + val_y*(lambda+2*mu);

  solution.v(0) = val_x*cp;
  solution.v(1) = val_y*cp;
"""
        )
