# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario


class TPV28(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single non-planar fault with two hills.
    On the plane, the initial stresses are defined by the surface.
    There are homogeneous initial stress conditions outside of the plane.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv28docs.html
    """

    domain_offset = [0.56, 0.0, 0.56]
    domain_size = [38.88, 38.88, 38.88]

    end_time = 13.0

    tracer_coordinates = [
        # off-fault at surface
        [17.0, 0.0, 5.0],
        [23.0, 0.0, 5.0],
        [17.0, 0.0, 20.0],
        [23.0, 0.0, 20.0],
        [17.0, 0.0, 35.0],
        [23.0, 0.0, 35.0],
    ]

    fault_coordinates = [
        # on-fault at surface (3)
        [20.0, 0.0, 5.0],
        [20.0, 0.0, 20.0],  # surface above fault hypocenter
        [20.0, 0.0, 35.0],
        # on-fault, general (21)
        [20.0, 6.0, 9.5],  # above left patch center
        [20.0, 7.5, 5.0],
        [20.0, 7.5, 8.0],
        [20.0, 7.5, 9.5],  # center of left patch
        [20.0, 7.5, 11.0],
        [20.0, 7.5, 14.0],
        [20.0, 9.0, 9.5],  # below left patch center
        [20.0, 3.0, 20.0],  # above fault hypocenter
        [20.0, 7.5, 20.0],  # on fault hypocenter
        [20.0, 12.0, 20.0],  # below fault hypocenter
        [20.0, 6.0, 30.5],  # above right patch center
        [20.0, 7.5, 26.0],
        [20.0, 7.5, 29.0],
        [20.0, 7.5, 30.5],  # center of right patch
        [20.0, 7.5, 32.0],
        [20.0, 7.5, 35.0],
        [20.0, 9.0, 30.5],  # below right patch center
    ]

    def domain_perturbations(self):
        return """
  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    20.0,
    [](tarch::la::Vector<DIMENSIONS, double> x) {

      // distance from center of first hill
      double r1 = sqrt(
          (x[1]-7.5)*(x[1]-7.5)
        + (x[2]-9.5)*(x[2]-9.5)
      );
      // distance from center of second hill
      double r2 = sqrt(
          (x[1]- 7.5)*(x[1]- 7.5)
        + (x[2]-30.5)*(x[2]-30.5)
      );
      if(r1<=3.){
        return -0.3*(1.0 + cos( 3.14159265359 * r1/3.0));
      }
      else if(r2<=3.){
        return -0.3*(1.0 + cos( 3.14159265359 * r2/3.0));
      }
      return 0.;
    }
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
      sxx = -60;
      syy = 0;
      szz = -60;
      sxy = 0;
      sxz = 29.380;
      syz = 0;
    """,
        "n_yz": """
      double r = sqrt((x[1]-7.5)*(x[1]-7.5)+(x[2]-20.0)*(x[2]-20.0));
      if (r <= 1.4) {
         return 11.60;
      }
      if (1.4 <= r && r <= 2.0){
         return 5.8 * (1.0 + cos(3.14159265359 * (r-1.4)/0.6));
      }
      return 0.0;""",
        "cohesion": "return 0;",
        "forced_rupture_time": "return 1000000000;",
        "d_c": "return 0.4;",
        # constant parameters
        "mu_d": 0.373,
        "mu_s": 0.677,
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 15,
        "f_cz": 20,
        "f_wz": 18,
    }
