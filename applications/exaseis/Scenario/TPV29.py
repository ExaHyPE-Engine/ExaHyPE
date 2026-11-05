# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario

import os


class TPV29(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single rough planar vertical strike-slip fault in a linear elastic medium.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv29_30docs.html
    """

    domain_offset = [
        -3.4,
        0.0,
        -3.4,
    ]  # The domain offset puts the fault at 20. on the cell boundary 13 cells into a level-3 grid of the Peano curve.
    domain_size = [
        48.6,
        48.6,
        48.6,
    ]  # The domain size gives a cell width of 0.6 on a level-4 grid of the Peano curve.

    end_time = 20.0

    tracer_coordinates = [
        # off-fault at surface
        [0.0, 0.0, 0.0],  # 0
        [40.0, 0.0, 0.0],  # 1
        [17.0, 0.0, 5.0],  # 2
        [23.0, 0.0, 5.0],  # 3
        [0.0, 0.0, 20.0],  # 4
        [17.0, 0.0, 20.0],  # 5
        [23.0, 0.0, 20.0],  # 6
        [40.0, 0.0, 20.0],  # 7
        [17.0, 0.0, 35.0],  # 8
        [23.0, 0.0, 35.0],  # 9
        [0.0, 0.0, 40.0],  # 10
        [40.0, 0.0, 40.0],  # 11
        # on-fault at surface (3)
        [20.0, 0.0, 15.0],  # surface above fault hypocenter
        [20.0, 0.0, 25.0],
        [20.0, 0.0, 35.0],
        # on-fault, general (21)
        [20.0, 15.6, 2.0],
        [20.0, 5.0, 5.0],
        [20.0, 12.0, 5.0],
        [20.0, 1.4, 9.0],
        [20.0, 10.1, 11.1],
        [20.0, 10.0, 15.0],  # on fault hypocenter
        [20.0, 16.0, 15.0],
        [20.0, 6.1, 15.8],
        [20.0, 12.0, 20.0],
        [20.0, 6.2, 14.3],
        [20.0, 6.0, 14.6],
        [20.0, 12.0, 25.0],
        [20.0, 5.7, 25.1],
        [20.0, 4.7, 25.9],
        [20.0, 15.3, 29.0],
        [20.0, 15.3, 29.3],
        [20.0, 5.0, 30.0],
        [20.0, 11.0, 30.0],
        [20.0, 13.0, 35.0],
        [20.0, 10.5, 36.7],
        [20.0, 4.5, 37.0],
    ]

    fault_coordinates = [
        # on-fault at surface (3)
        [20.0, 0.0, 15.0],  # surface above fault hypocenter
        [20.0, 0.0, 25.0],
        [20.0, 0.0, 35.0],
        # on-fault, general (21)
        [20.0, 15.6, 2.0],
        [20.0, 5.0, 5.0],
        [20.0, 12.0, 5.0],
        [20.0, 1.4, 9.0],
        [20.0, 10.1, 11.1],
        [20.0, 10.0, 15.0],  # on fault hypocenter
        [20.0, 16.0, 15.0],
        [20.0, 6.1, 15.8],
        [20.0, 12.0, 20.0],
        [20.0, 6.2, 14.3],
        [20.0, 6.0, 14.6],
        [20.0, 12.0, 25.0],
        [20.0, 5.7, 25.1],
        [20.0, 4.7, 25.9],
        [20.0, 15.3, 29.0],
        [20.0, 15.3, 29.3],
        [20.0, 5.0, 30.0],
        [20.0, 11.0, 30.0],
        [20.0, 13.0, 35.0],
        [20.0, 10.5, 36.7],
        [20.0, 4.5, 37.0],
    ]

    def domain_perturbations(self):
        return """
  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    20.0,
    [](const tarch::la::Vector<DIMENSIONS, double> x) {
      // The file extends the perturbation beyond the edges of the fault. The extension smooths the
      // gradients at the edges.
      static tarch::reader::NetCDFFieldSampler perturbationSampler(
        {
          {"perturbation", "tpv29-fault.nc"}
        },
        {50., 40.},
        {-5., -5.}
      );
      return 0.001 * perturbationSampler.samplePiecewiseConstant("perturbation", {x[2], x[1]});
    }
  );
"""

    def initial_conditions(self):
        return """
      Q.rho = 2.67;
      Q.cp  = 6.0;
      Q.cs  = 3.464;
"""

    def generate_required_files(self):
        super().generate_required_files()

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__)) + "/specs/TPV29/tpv29-fault.nc",
            "tpv29-fault.nc",
        )

        return

    dynamicRuptureDictionary = {
        "initialStresses": """
  double depthTaper = 0.;
  if (x[1] <= 17.) {
    depthTaper = 1.;
  } else if (x[1] <= 22.) {
    depthTaper = (22. - x[1]) / 5.;
  }
  const double verticalStress = -2.670 * 9.8 * x[1];
  const double porePressure   = 1.000 * 9.8 * x[1];

  constexpr double B11 = 1.025837;
  constexpr double B13 = -0.158649;
  constexpr double B33 = 0.974162;

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
        "cohesion": "return 0.40 + 0.2 * std::fmax(4.0 - x[1], 0);",
        "forced_rupture_time": """
      const double     distance         = std::sqrt(std::pow(x[2] - 15.0, 2.0) + std::pow(x[1] - 10.0, 2.0));
      constexpr double CriticalDistance = 4.0;
      constexpr double ShearWaveSpeed   = 3.4640;
      if (distance <= CriticalDistance) {
        return distance / (0.7 * ShearWaveSpeed)
             + (0.081 * CriticalDistance / (0.7 * ShearWaveSpeed))
                 * (1.0 / (1.0 - (distance / CriticalDistance) * (distance / CriticalDistance)) - 1.0);
      }
      return 100000000.0;""",
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
