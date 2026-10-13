# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario

import os


class TPV16(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Spontaneous rupture on a vertical strike-slip fault in a homogeneous halfspace.
    There are randomly-generated heterogeneous initial stress conditions.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv16_17docs.html
    """

    domain_offset = [-5.92, 0.0, -5.92]  # centers the fault which is at 20.
    domain_size = [
        51.84,
        51.84,
        51.84,
    ]  # The domain size leaves one cell outside the fault on each side on a level-3 grid of the Peano curve.

    end_time = 15.0

    auxiliary_variables = {"initialShearStress": 1, "criticalSlipDistance": 1}

    tracer_coordinates = [
        [26.0, 0.0, 11.0],
        [26.0, 0.0, 20.0],
        [26.0, 0.0, 29.0],
        [20.4, 0.0, 11.0],
        [20.4, 0.0, 20.0],
        [20.4, 0.0, 29.0],
        [19.6, 0.0, 11.0],
        [19.6, 0.0, 20.0],
        [19.6, 0.0, 29.0],
        [14.0, 0.0, 11.0],
        [14.0, 0.0, 20.0],
        [14.0, 0.0, 29.0],
    ]

    fault_coordinates = [
        [20.0, 0.0, 11.0],
        [20.0, 0.0, 20.0],
        [20.0, 0.0, 29.0],
        [20.0, 9.0, 11.0],
        [20.0, 9.0, 20.0],
        [20.0, 9.0, 29.0],
        [20.0, 0.0, 11.0],
        [20.0, 0.0, 20.0],
        [20.0, 0.0, 29.0],
        [20.0, 9.0, 11.0],
        [20.0, 9.0, 20.0],
        [20.0, 9.0, 29.0],
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
      // The file stores the coordinate along the fault first and the depth second.
      static tarch::reader::NetCDFFieldSampler<2> faultSampler(
        {
          {"stress", "tpv16-parameters.nc", "stress"},
          {"slip", "tpv16-parameters.nc", "slip"}
        },
        {48.0, 19.5},
        {-4.0, 0.0}
      );

      Q[Shortcuts::rho] = 2.67;
      Q[Shortcuts::cp ] = 6.0;
      Q[Shortcuts::cs ] = 3.464;

      Q[Shortcuts::initialShearStress] = faultSampler.samplePiecewiseConstant(
        "stress",
        {x[2], x[1]}
      );

      Q[Shortcuts::criticalSlipDistance] = 0.4;
      if (std::abs(x[1] - 20.0) < 24.0 and x[1] < 19.5) {
        Q[Shortcuts::criticalSlipDistance] = faultSampler.samplePiecewiseConstant(
          "slip",
          {x[2], x[1]}
        );
      }
"""

    def generate_required_files(self):
        super().generate_required_files()

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))
            + "/specs/TPV16/tpv16-parameters.nc",
            "tpv16-parameters.nc",
        )

        return

    dynamicRuptureDictionary = {
        "initialStresses": """
  sxx = -60;
  syy = 0;
  szz = 0;
  sxy = 0;
  sxz = Q[Shortcuts::initialShearStress];
  syz = 0;
    """,
        "n_yz": "return 0;",
        "cohesion": """
  if(x[1]<3){
    return 4-(4*x[1])/3;
  }
  return 0;""",
        "forced_rupture_time": "return 9999999;",
        "d_c": "return Q[Shortcuts::criticalSlipDistance];",
        # constant parameters
        "mu_d": 0.373,
        "mu_s": 0.677,  # Should increase in locations where the initial shear stress is very high according to documentation
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 19.5,
        "f_cz": 20,
        "f_wz": 24,
    }
