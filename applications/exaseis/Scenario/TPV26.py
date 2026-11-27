# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario


class TPV26(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single planar vertical strike-slip fault in a linear elastic medium.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv26_27docs.html
    """

    domain_offset = [
        -1.6,
        0.0,
        -1.6,
    ]  # The domain offset puts a buffer of three cells before and after the fault on a level-4 grid of the Peano curve.
    domain_size = [
        43.2,
        43.2,
        43.2,
    ]  # The domain size gives a cell width of 0.5333 on a level-4 grid of the Peano curve.

    end_time = 20.0

    tracer_coordinates = [
        # off-fault at surface (18 total)
        [0.0, 0.0, 0.0],
        [10.0, 0.0, 0.0],
        [30.0, 0.0, 0.0],
        [40.0, 0.0, 0.0],
        [17.0, 0.0, 15.0],
        [23.0, 0.0, 15.0],
        [0.0, 0.0, 20.0],
        [10.0, 0.0, 20.0],
        [30.0, 0.0, 20.0],
        [40.0, 0.0, 20.0],
        [17.0, 0.0, 25.0],
        [23.0, 0.0, 25.0],
        [17.0, 0.0, 35.0],
        [23.0, 0.0, 35.0],
        [0.0, 0.0, 40.0],
        [10.0, 0.0, 40.0],
        [30.0, 0.0, 40.0],
        [40.0, 0.0, 40.0],
    ]

    fault_coordinates = [
        # on-fault at surface (2 total)
        [20.0, 0.0, 15.0],
        [20.0, 0.0, 35.0],
        # on-fault at hypocenter depth (6 total)
        [20.0, 10.0, 5.0],
        [20.0, 10.0, 15.0],
        [20.0, 10.0, 20.0],
        [20.0, 10.0, 25.0],
        [20.0, 10.0, 30.0],
        [20.0, 10.0, 35.0],
        # on-fault off hypocenter depth (4 total)
        [20.0, 5.0, 15.0],
        [20.0, 5.0, 35.0],
        [20.0, 15.0, 15.0],
        [20.0, 15.0, 35.0],
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
      Q.rho = 2.67;
      Q.cp  = 6.0;
      Q.cs  = 3.464;
"""

    dynamicRuptureDictionary = {
        "initialStresses": """
  double depthTaper = 0.;
  if (x[1] <= 15.) {
    depthTaper = 1.;
  } else if (x[1] <= 20.) {
    depthTaper = (20. - x[1]) / 5.;
  }

  const double porePressure   = 1.000 * 9.8 * x[1];
  const double verticalStress = -2.670 * 9.8 * x[1];

  constexpr double B11 = 0.926793;
  constexpr double B13 = -0.169029;
  constexpr double B33 = 1.073206;

  sxx = depthTaper * (B33 * (verticalStress + porePressure) - porePressure)
      + (1.0 - depthTaper) * verticalStress + porePressure;
  syy = verticalStress + porePressure;
  szz = depthTaper * (B11 * (verticalStress + porePressure) - porePressure)
      + (1.0 - depthTaper) * verticalStress + porePressure;
  sxy = 0;
  sxz = depthTaper * (B13 * (verticalStress + porePressure));
  syz = 0;
    """,
        "n_yz": "return 0;",
        "cohesion": "return 0.40 + 0.72 * std::fmax(5.0 - x[1], 0);",
        "forced_rupture_time": """
  const double     distance         = std::sqrt(std::pow(x[2] - 15.0, 2.0) + std::pow(x[1] - 10.0, 2.0));
  constexpr double CriticalDistance = 4.0;
  constexpr double ShearWaveSpeed   = 3.4640;
  if (distance <= CriticalDistance) {
    return distance / (0.7 * ShearWaveSpeed)
         + (0.081 * CriticalDistance / (0.7 * ShearWaveSpeed))
             * (1.0 / (1.0 - std::pow(distance / CriticalDistance, 2.0)) - 1.0);
  }
  return 1000000000.0;""",
        "d_c": "return 0.3;",
        # constant parameters
        "mu_d": 0.12,
        "mu_s": 0.18,
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 20,
        "f_cz": 20,
        "f_wz": 20,
    }
