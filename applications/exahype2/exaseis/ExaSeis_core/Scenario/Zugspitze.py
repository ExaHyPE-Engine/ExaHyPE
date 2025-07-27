# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class Zugspitze(Scenario):
    """
    Benchmark representing a rectangular area of 80x80km around the Zugspitze.
    The scenario has no analytical solution, but the complex topography leads to
    complicated scattering on the surface, which makes for an interesting benchmark.

    Mentioned e.g. in "A stable discontinuous Galerkin method for linear elastodynamics in 3D geometrically complex elastic solids using physics based numerical fluxes, Computer Methods in Applied Mechanics and Engineering"
    by Kenneth Duru, Leonhard Rannabauer, Alice-Agnes Gabriel, On Ki Angel Ling, Heiner Igel, Michael Bader
    (available at https://doi.org/10.1016/j.cma.2021.114386)
    """

    domain_offset   = [4358.459, 0.0, 2661.23]
    domain_size     = [80.85, 80.85, 80.85]

    end_time = 30.0

    tracer_coordinates = [
        [4370.0, 0.0, 2675.0], [4395.0, 0.0, 2700.0], [4405.0, 0.0, 2670.0],
        [4405.0, 0.0, 2710.0], [4375.0, 0.0, 2708.0], [4424.756, 0.0, 2675.783],
        [4424.756,  11.320, 2675.783]
    ]

    def initial_conditions(self):
        return """
        Q[Shortcuts::rho] = 2.67;
        Q[Shortcuts::cp ] = 6.0;
        Q[Shortcuts::cs ] = 3.464;
"""

    #TODO: is this point source correct?
    #it is in the scenario of ExaHyPE1 but also does not use certain parameters
    # and is a 1-to-1 copy of LOH1
    def point_source(self):
        return """
  std::fill_n(forceVector, 9, 0.);

  auto jacobian = Q[Shortcuts::jacobian];

  assertion2(n == 0, "Only a single pointSource for Zugspitze",n);

  constexpr  double pi = 3.14159265359;
  constexpr  double sigma = 0.1149;
  constexpr  double t0 = 0.1;
  constexpr  double M0 = 1000.0;
  
  double f =  M0*t/(t0*t0)*std::exp(-t/t0);
  
  forceVector[Shortcuts::sigma + 4] = f;

  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = forceVector[i] / jacobian;
  }
"""
    def init_point_source(self):
        return """
  pointSourceLocation[0][0] = 4424.756;
  pointSourceLocation[0][1] = 10.0; // or 11.320? ExaHyPE1 has this commented out
  pointSourceLocation[0][2] = 2675.783;

  context->correctPointSourceLocation(pointSourceLocation);
"""

    def generate_required_files(self, order):

        dictionary = { "MODE": order+1 }
        self.generate_file_from_template(
            os.path.dirname(os.path.realpath(__file__))+"/specs/Zugspitze/zugspitze.yaml.template", 
            "zugspitze.yaml", dictionary
        )

        #using a symlink instead of a copy as the file is quite large
        try:
            os.symlink(
                os.path.dirname(os.path.realpath(__file__))+"/specs/Zugspitze/zugspitze_25m_4345_100_2650_100.nc", 
                "zugspitze_25m_4345_100_2650_100.nc"
            )
        except:
            print("creating symlink failed, assume the file already exists")

        return