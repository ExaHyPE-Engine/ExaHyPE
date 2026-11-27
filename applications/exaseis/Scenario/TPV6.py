# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario


class TPV6(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Spontaneous rupture on a vertical strike-slip fault in a bimaterial halfspace,
    with high shear modulus contrast across the fault (a "well-posed" problem).

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv6docs.html
    """

    domain_offset = [3.8, 0.0, 3.8]  # centers the fault which is at 20.
    domain_size = [
        32.4,
        32.4,
        32.4,
    ]  # The domain size leaves one cell outside the fault on each side on a level-3 grid of the Peano curve.

    end_time = 10.0

    tracer_coordinates = []

    fault_coordinates = [
        # Surface
        [20.0, 0.0, 8.0],
        [20.0, 0.0, 20.0],
        [20.0, 0.0, 32.0],
        # Underground
        [20.0, 7.5, 8.0],
        [20.0, 7.5, 32.0],
    ]

    def domain_perturbations(self):
        return """
  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    20.0,
    []([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double> x) { return 0; }
  );
"""

    def initial_conditions(self):
        return """
      // The material on the far side of the fault at x = 20 differs from the near side.
      constexpr double EdgeTolerance = 1e-8;
      if (x[0] >= 20.0 - EdgeTolerance) {
        Q.rho = 2.225;
        Q.cp  = 3.75;
        Q.cs  = 2.165;
      } else {
        Q.rho = 2.67;
        Q.cp  = 6.0;
        Q.cs  = 3.464;
      }
"""

    dynamicRuptureDictionary = {
        "initialStresses": """
      sxx = -120;
      syy = 0;
      szz = 0;
      sxy = 0;
      sxz = 70;
      syz = 0;
    """,
        "n_yz": """
      constexpr double EdgeTolerance = 1e-8;
      if (
        6.0 - EdgeTolerance <= x[1] and x[1] <= 9.0 + EdgeTolerance and 18.5 - EdgeTolerance <= x[2]
        and x[2] <= 21.5 + EdgeTolerance
      ) {
        return 11.6;
      }
      return 0.0;""",
        "cohesion": "return 0;",
        "forced_rupture_time": "return 9999999;",
        "d_c": "return 0.4;",
        # constant parameters
        "mu_d": 0.525,
        "mu_s": 0.677,
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 15,
        "f_cz": 20,
        "f_wz": 15,
    }
