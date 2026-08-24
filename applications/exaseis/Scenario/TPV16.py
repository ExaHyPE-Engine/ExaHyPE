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

    domain_offset   = [-5.92,  0., -5.92] #centers the fault which is at 20.
    domain_size     = [51.84, 51.84, 51.84]  #leads to one cell outside of fault on each side (with 27 cells)

    end_time = 15.0

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
      [14.0, 0.0, 29.0]
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
      [20.0, 9.0, 29.0]
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

    def generate_required_files(self):
        super().generate_required_files()

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV16/tpv16-parameters.nc",
            "tpv16-parameters.nc"
        )

        return


    dynamicRuptureDictionary = {
    "initialStresses": """
  static tarch::reader::NetCDFFieldSampler netcdfSampler(
    {
      {"stress", "tpv16-parameters.nc", "stress"}
    },
    {48.,  // size[0], here along-fault
    19.5}, // size[1], here vertical
    {-4., 0.}
  );
  sxx = -60;
  syy = 0;
  szz = 0;
  sxy = 0;
  sxz = netcdfSampler.samplePiecewiseConstant(
    "stress",
    {x[2], x[1]}
  );
  syz = 0;
    """,
      "n_yz": "return 0;",
      "cohesion": """
  if(x[1]<3){
    return 4-(4*x[1])/3;
  }
  return 0;""",
      "forced_rupture_time": "return 9999999;",
      "d_c": """
if(std::abs(x[1]-20.)<24. && x[1]<19.5){
  static tarch::reader::NetCDFFieldSampler netcdfSampler(
    {
      {"slip", "tpv16-parameters.nc", "slip"}
    },
    {48.,  // size[0], here along-fault
    19.5}, // size[1], here vertical
    {-4., 0.}
  );
  return netcdfSampler.samplePiecewiseConstant(
    "slip",
    {x[2], x[1]}
  );
}
return 0.4;""",
  #constant parameters
      "mu_d": 0.373,
      "mu_s": 0.677, #Should increase in locations where the initial shear stress is very high according to documentation
      "t0_rupture": 0.5,
      "f_cy": 0,
      "f_wy": 19.5,
      "f_cz": 20,
      "f_wz": 24
    }
