# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario


class HHS1(Scenario):
    """
    The Sismowine benchmark HHS1: A point source in a homogeneous medium tests the modelling of a
    planar free surface (https://sismowine.org/model/WP1_HHS1.pdf).
    """

    domain_offset = [-2.025, 0.0, -2.025]
    domain_size = [12.15, 12.15, 12.15]

    end_time = 10.0

    tracer_coordinates = [
        [0.000, 0.0, 0.693],
        [0.000, 0.0, 5.543],
        [0.000, 0.0, 10.392],
        [0.490, 0.0, 0.490],
        [3.919, 0.0, 3.919],
        [7.348, 0.0, 7.3480],
        [0.577, 0.0, 0.384],
        [4.612, 0.0, 3.075],
        [8.647, 0.0, 5.7640],
    ]

    def initial_conditions(self):
        return """
      Q.rho = 2.7;
      Q.cp  = 6.0;
      Q.cs  = 3.464;
"""

    def point_source(self):
        return """
  ASSERTION(n == 0, n);
  constexpr double t0                 = 0.1;
  constexpr double M0                 = 1000.0;
  const double     sourceTimeFunction = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector             = {};
  forceVector.sigma(0, 2) = sourceTimeFunction;
  forceVector.sigma(0, 2) /= Q.jacobian;
"""

    def init_point_source(self):
        return """
  location[0] = _curvi->invertProjection({0., 2., 0.});
"""
