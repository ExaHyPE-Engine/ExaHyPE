# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario

import os


class TPV34(DynamicRuptureScenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Imperial Fault, Model 1. Spontaneous rupture on a vertical strike-slip fault.
    There is a 3D velocity structure in a linear elastic half-space. The velocity structure is derived from
    SCEC Community Velocity Model CVM-H in the vicinity of the Imperial Fault.
    Initial shear and normal stresses are proportional to the shear modulus.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv34docs.html
    """

    domain_offset = [3.8, 0.0, 3.8]  # centers the fault which is at 20.
    domain_size = [
        32.4,
        32.4,
        32.4,
    ]  # The domain size gives a cell width of 0.333 on a level-4 grid of the Peano curve.

    end_time = 20.0

    auxiliary_variables = {"faultShearModulus": 1}

    tracer_coordinates = [
        [17.0, 0.0, 10.0],
        [17.0, 2.4, 10.0],
        [17.0, 0.0, 0.0],
        [17.0, 2.4, 0.0],
        [17.0, 0.0, 20.0],
        [17.0, 2.4, 20.0],
        [17.0, 0.0, 30.0],
        [17.0, 2.4, 30.0],
        [17.0, 0.0, 40.0],
        [17.0, 2.4, 40.0],
        [11.0, 0.0, 10.0],
        [11.0, 2.4, 10.0],
        [11.0, 0.0, 0.0],
        [11.0, 2.4, 0.0],
        [11.0, 0.0, 20.0],
        [11.0, 2.4, 20.0],
        [11.0, 0.0, 30.0],
        [11.0, 2.4, 30.0],
        [11.0, 0.0, 40.0],
        [11.0, 2.4, 40.0],
        [5.0, 0.0, 5.0],
        [5.0, 2.4, 5.0],
        [5.0, 0.0, 20.0],
        [5.0, 2.4, 20.0],
        [5.0, 0.0, 35.0],
        [5.0, 2.4, 35.0],
        [20.0, 0.0, 0.0],
        [20.0, 2.4, 0.0],
        [20.0, 0.0, 40.0],
        [20.0, 2.4, 40.0],
        [23.0, 0.0, 10.0],
        [23.0, 2.4, 10.0],
        [23.0, 0.0, 0.0],
        [23.0, 2.4, 0.0],
        [23.0, 0.0, 20.0],
        [23.0, 2.4, 20.0],
        [23.0, 0.0, 30.0],
        [23.0, 2.4, 30.0],
        [23.0, 0.0, 40.0],
        [23.0, 2.4, 40.0],
        [29.0, 0.0, 10.0],
        [29.0, 2.4, 10.0],
        [29.0, 0.0, 0.0],
        [29.0, 2.4, 0.0],
        [29.0, 0.0, 20.0],
        [29.0, 2.4, 20.0],
        [29.0, 0.0, 30.0],
        [29.0, 2.4, 30.0],
        [29.0, 0.0, 40.0],
        [29.0, 2.4, 40.0],
        [35.0, 0.0, 5.0],
        [35.0, 2.4, 5.0],
        [35.0, 0.0, 20.0],
        [35.0, 2.4, 20.0],
        [35.0, 0.0, 35.0],
        [35.0, 2.4, 35.0],
    ]

    fault_coordinates = [
        [20.0, 0.0, 14.0],
        [20.0, 1.0, 14.0],
        [20.0, 2.4, 14.0],
        [20.0, 5.0, 14.0],
        [20.0, 7.5, 14.0],
        [20.0, 10.0, 14.0],
        [20.0, 12.0, 14.0],
        [20.0, 0.0, 8.0],
        [20.0, 1.0, 8.0],
        [20.0, 2.4, 8.0],
        [20.0, 5.0, 8.0],
        [20.0, 7.5, 8.0],
        [20.0, 10.0, 8.0],
        [20.0, 12.0, 8.0],
        [20.0, 0.0, 20.0],
        [20.0, 1.0, 20.0],
        [20.0, 2.4, 20.0],
        [20.0, 5.0, 20.0],
        [20.0, 7.5, 20.0],
        [20.0, 10.0, 20.0],
        [20.0, 12.0, 20.0],
        [20.0, 0.0, 26.0],
        [20.0, 1.0, 26.0],
        [20.0, 2.4, 26.0],
        [20.0, 5.0, 26.0],
        [20.0, 7.5, 26.0],
        [20.0, 10.0, 26.0],
        [20.0, 12.0, 26.0],
        [20.0, 0.0, 32.0],
        [20.0, 1.0, 32.0],
        [20.0, 2.4, 32.0],
        [20.0, 5.0, 32.0],
        [20.0, 7.5, 32.0],
        [20.0, 10.0, 32.0],
        [20.0, 12.0, 32.0],
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
        static tarch::reader::NetCDFFieldSampler<3> materialSampler(
          {
            {"rho", "tpv34-material.nc", "rho", {"x", "y", "z"}},
            {"cp",  "tpv34-material.nc", "cp",  {"x", "y", "z"}},
            {"cs",  "tpv34-material.nc", "cs",  {"x", "y", "z"}}
          },
          {DomainSize[0], DomainSize[1], DomainSize[2]},
          {DomainOffset[0], DomainOffset[1], DomainOffset[2]}
        );

        Q[Shortcuts::rho] = materialSampler.samplePiecewiseConstant(
          "rho",
          {x[0], x[1], x[2]}
        );

        Q[Shortcuts::cp]  = materialSampler.samplePiecewiseConstant(
          "cp",
          {x[0], x[1], x[2]}
        );

        Q[Shortcuts::cs]  = materialSampler.samplePiecewiseConstant(
          "cs",
          {x[0], x[1], x[2]}
        );

        static tarch::reader::NetCDFFieldSampler<2> faultSampler(
          {
            {"stress", "tpv34-mu.nc", "stress"}
          },
          {DomainSize[2], DomainSize[1]},
          {DomainOffset[2], DomainOffset[1]}
        );

        Q[Shortcuts::faultShearModulus] = faultSampler.samplePiecewiseConstant(
          "stress",
          {x[2], x[1]}
        );
"""

    def generate_required_files(self):
        super().generate_required_files()

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__)) + "/specs/TPV34/tpv34-mu.nc",
            "tpv34-mu.nc",
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))
            + "/specs/TPV34/tpv34-material.nc",
            "tpv34-material.nc",
        )

        return

    mu_compute = """
  const Value mu = Q[Shortcuts::faultShearModulus];
    """

    dynamicRuptureDictionary = {
        "initialStresses": mu_compute
        + """
  sxx = -60.0 * mu;
  syy = 0;
  szz = 0;
  sxy = 0;
  sxz = 30.0 * mu;
  syz = 0;
    """,
        "n_yz": mu_compute
        + """
  double r = sqrt((x[1]-7.5)*(x[1]-7.5)+(x[2]-20.)*(x[2]-20.));
  if (r <= 1.4) {
    return 4.95 * mu;
  }
  if (1.4 <= r && r <= 2.0){
    return 2.475 * (1.0 + cos(std::numbers::pi * (r-1.4)/0.6)) * mu;
  }
  return 0.0;""",
        "cohesion": """
  if(x[1] < 2.4) {
    return 0.425 * (2.4 - x[1]);
  }
  return 0;
""",
        "forced_rupture_time": "return 9999999;",
        "d_c": "return 0.18;",
        # constant parameters
        "mu_d": 0.450,
        "mu_s": 0.580,
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 15,
        "f_cz": 20,
        "f_wz": 15,
    }
