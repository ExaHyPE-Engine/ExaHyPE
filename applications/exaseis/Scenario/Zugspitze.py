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

    def domain_perturbations(self):
        return """
  _curvi->applyPerturbation(
    1,
    [](tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler netcdfSampler(
        {
          {"perturbation", "zugspitze_25m_4345_100_2650_100.nc"}
        },
        {DomainSize[0], DomainSize[2]},
        {DomainOffset[0], DomainOffset[2]}
      );
      return -0.001*netcdfSampler.samplePiecewiseConstant(
        "perturbation",
        {x[0], x[2]}
      );
    }
  );
"""

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

  assertion(n == 0, "Only a single pointSource for Zugspitze",n);

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
  tarch::la::Vector<DIMENSIONS, double> ps = _curvi->invertProjection({
    4424.756,
    10.0, // or 11.320? ExaHyPE1 has this commented out
    2675.783
  });

  pointSourceLocation[0][0] = ps[0];
  pointSourceLocation[0][1] = ps[1];
  pointSourceLocation[0][2] = ps[2];
"""

    def generate_required_files(self):
        #using a symlink instead of a copy as the file is quite large
        source = os.path.dirname(os.path.realpath(__file__))+"/specs/Zugspitze/zugspitze_25m_4345_100_2650_100.nc"
        target = "zugspitze_25m_4345_100_2650_100.nc"

        # os.symlink happily creates a dangling link, so check the topography is actually there.
        # Otherwise the run only fails much later, inside the NetCDF reader.
        if not os.path.exists(source):
            raise FileNotFoundError(
                "The Zugspitze topography " + source + " is missing, so the scenario cannot be built."
            )

        try:
            os.symlink(source, target)
        except FileExistsError:
            # A previous invocation already linked it into the build folder
            pass

        return