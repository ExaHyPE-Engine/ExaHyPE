# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class HHS1(Scenario):
    """
    Part of a series of benchmarks by the SeISmic MOdeling Web INterfacE (Sismowine)

    Single point-source in a homogeneous medium.
    Purpose: Assess the precision of modeling a planar free surface

    The description of the scenario can be found at: https://sismowine.org/model/WP1_HHS1.pdf
    """

    domain_offset   = [-2.025, 0.0, -2.025]
    domain_size     = [12.15, 12.15, 12.15]

    end_time = 10.0

    tracer_coordinates = [
        [0.000, 0., 0.693], [0.000, 0., 5.543], [0.000, 0., 10.392],
        [0.490, 0., 0.490], [3.919, 0., 3.919], [7.348, 0., 7.3480],
        [0.577, 0., 0.384], [4.612, 0., 3.075], [8.647, 0., 5.7640]
    ]

    def initial_conditions(self):
        return """
        Q[Shortcuts::rho] = 2.7;
        Q[Shortcuts::cp ] = 6.0;
        Q[Shortcuts::cs ] = 3.464;
"""

    def point_source():
        return """
  std::fill_n(forceVector, 9, 0.);

  auto jacobian = Q[Shortcuts::jacobian];

  assertion2(n == 0, "Only a single pointSource for HHS1",n);

  constexpr double t0 = 0.1;
  constexpr double M0 = 1000.0;
  
  double f = M0*t/(t0*t0)*std::exp(-t/t0);
  
  forceVector[Shortcuts::sigma + 4] = f;

  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = forceVector[i] / jacobian;
  }
"""
    def init_point_source():
        return """
  pointSourceLocation[0][0] = 0.0;
  pointSourceLocation[0][1] = 2.0;
  pointSourceLocation[0][2] = 0.0;

  context->correctPointSourceLocation(pointSourceLocation);
"""
    
    def generate_required_files(self, order):

        dictionary = { "MODE": order+1 }
        self.generate_file_from_template(

            os.path.dirname(os.path.realpath(__file__))+"/LOH1/loh1.yaml.template",
            "hhs1.yaml", dictionary
        )

        return