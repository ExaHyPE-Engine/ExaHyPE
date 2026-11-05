# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os


class Zugspitze(Scenario):
    """
    An area of 80 km by 80 km around the Zugspitze, whose topography scatters the waves at the
    surface. Duru et al. use it in "A stable discontinuous
    Galerkin method for linear elastodynamics in 3D geometrically complex elastic solids using
    physics based numerical fluxes" (https://doi.org/10.1016/j.cma.2021.114386).
    """

    domain_offset = [4358.459, 0.0, 2661.23]
    domain_size = [80.85, 80.85, 80.85]

    end_time = 30.0

    tracer_coordinates = [
        [4370.0, 0.0, 2675.0],
        [4395.0, 0.0, 2700.0],
        [4405.0, 0.0, 2670.0],
        [4405.0, 0.0, 2710.0],
        [4375.0, 0.0, 2708.0],
        [4424.756, 0.0, 2675.783],
        [4424.756, 11.320, 2675.783],
    ]

    def domain_perturbations(self):
        return """
  _curvi->applyPerturbation(
    1,
    [](const tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler topographySampler(
        {
          {"perturbation", "zugspitze_25m_4345_100_2650_100.nc"}
        },
        {DomainSize[0], DomainSize[2]},
        {DomainOffset[0], DomainOffset[2]}
      );
      return -0.001*topographySampler.samplePiecewiseConstant(
        "perturbation",
        {x[0], x[2]}
      );
    }
  );
"""

    def initial_conditions(self):
        return """
      Q.rho = 2.67;
      Q.cp  = 6.0;
      Q.cs  = 3.464;
"""

    # The point source is a copy of the LOH1 point source.
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
  location[0] = _curvi->invertProjection({4424.756, 10.0, 2675.783});
"""

    def generate_required_files(self):
        source = (
            os.path.dirname(os.path.realpath(__file__))
            + "/specs/Zugspitze/zugspitze_25m_4345_100_2650_100.nc"
        )
        target = "zugspitze_25m_4345_100_2650_100.nc"

        # os.symlink() creates a dangling link for a missing source.
        if not os.path.exists(source):
            raise FileNotFoundError(
                "The Zugspitze scenario reads the topography file '"
                + source
                + "'. The command 'git lfs pull' downloads it."
            )

        try:
            os.symlink(source, target)
        except FileExistsError:
            # A previous invocation already linked it into the build folder.
            pass

        return
