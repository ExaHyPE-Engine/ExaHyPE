# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario

class TPV5(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Spontaneous rupture on a vertical strike-slip fault in a homogeneous halfspace.
    There are slightly heterogeneous initial stress conditions.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv5docs.html
    """

    domain_offset   = [3.8, 0.0, 3.8] #centers the fault which is at 20.
    domain_size     = [32.4, 32.4, 32.4] #The domain size leaves one cell outside the fault on each side on a level-3 grid of the Peano curve.

    end_time = 10.0

    tracer_coordinates = [
          # off-fault at surface
          [17.0, 0.0,  8.0],
          [17.0, 0.0, 32.0],
          [23.0, 0.0,  8.0],
          [23.0, 0.0, 32.0],
          # off-fault at hypocenter depth
          [17.0, 7.5,  8.0],
          [17.0, 7.5, 32.0],
          [23.0, 7.5,  8.0],
          [23.0, 7.5, 32.0],
    ]

    fault_coordinates = [
          # on-fault at surface
          [20.0, 0.0,  8.0],
          [20.0, 0.0, 12.5],
          [20.0, 0.0, 15.5],
          [20.0, 0.0, 20.0], #surface above fault hypocenter
          [20.0, 0.0, 24.5],
          [20.0, 0.0, 27.5],
          [20.0, 0.0, 32.0],
          [20.0, 3.0, 20.0], #above fault hypocenter
          [20.0, 12.0, 20.0], #below fault hypocenter
          #on-fault at hypocenter depth
          [20.0, 7.5,  8.0],
          [20.0, 7.5, 12.5], #center of left patch
          [20.0, 7.5, 15.5],
          [20.0, 7.5, 20.0], #on fault hypocenter
          [20.0, 7.5, 24.5],
          [20.0, 7.5, 27.5], #center of right patch
          [20.0, 7.5, 32.0]
    ]

    def domain_perturbations(self):
        return """
  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    20.0,
    [](tarch::la::Vector<DIMENSIONS, double> x) { return 0; }
  );
"""

    def initial_conditions(self):
        return """
      Q[Shortcuts::rho] = 2.67;
      Q[Shortcuts::cp ] = 6.0;
      Q[Shortcuts::cs ] = 3.464;
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
    if ((6 <= x[1]) && (x[1] <= 9)) {
      if ((11 <= x[2]) && (x[2] <= 14)) {
        return 8;
      }
      if ((18.5 <= x[2]) && (x[2] <= 21.5)) {
        return 11.6;
      }
      if ((26 <= x[2]) && (x[2] <= 29)) {
        return -8;
      }
    }
    return 0;""",
      "cohesion": "return 0;",
      "forced_rupture_time": "return 9999999;",
      "d_c": "return 0.4;",
  #constant parameters
      "mu_d": 0.525,
      "mu_s": 0.677,
      "t0_rupture": 0.5,
      "f_cy": 0,
      "f_wy": 15,
      "f_cz": 20,
      "f_wz": 15
    }
