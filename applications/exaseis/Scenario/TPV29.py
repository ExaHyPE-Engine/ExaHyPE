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
    [](tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler netcdfSampler(
        {
          {"perturbation", "tpv29-fault.nc"}
        },
        {50., 40.},
        {-5., -5.}
      );

  // The NetCDF file contains extended boundaries which serve to smooth out gradients caused by the edges of the fault,
  // these can be disabled by uncommenting the corresponding sections of code

//      if(x[0]>0. && x[0]<40. && x[1]<20.){
        return 0.001*netcdfSampler.samplePiecewiseConstant(
          "perturbation",
          {x[2], x[1]}
        );
//      }
//      else{
//        return 0.;
//      }

    }
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
            os.path.dirname(os.path.realpath(__file__)) + "/specs/TPV29/tpv29-fault.nc",
            "tpv29-fault.nc",
        )

        return

    dynamicRuptureDictionary = {
        "initialStresses": """
  double Omega = 0.;
  if(x[1]<=17.){
    Omega=1.;
  }
  else if(x[1]<=22.){
    Omega = (22.-x[1])/5.;
  }
  const double s_zz0  = -2.670 * 9.8 * x[1];
  const double Pf     =  1.000 * 9.8 * x[1];

  constexpr double b_11 = 1.025837;
  constexpr double b_13 = -0.158649;
  constexpr double b_33 = 0.974162;

  sxx = Omega*(b_33*(s_zz0+Pf)-Pf)+(1.0-Omega)*s_zz0 + Pf;
  syy = s_zz0 + Pf;
  szz = Omega*(b_11*(s_zz0+Pf)-Pf)+(1.0-Omega)*s_zz0 + Pf;
  sxy = 0;
  sxz = Omega*(b_13*(s_zz0+Pf));
  syz = 0;
    """,
        "n_yz": "return 0;",
        "cohesion": "return 0.40 + 0.2 * std::fmax(4.0 - x[1],0);",
        "forced_rupture_time": """
      double r = std::sqrt(std::pow(x[2]-15.0, 2.0) + pow(x[1]-10.0, 2.0));
      double r_crit = 4.0;
      double Vs = 3.4640;
      if (r <= r_crit) {
        return r/(0.7*Vs) + (0.081*r_crit/(0.7*Vs)) * (1.0/(1.0-(r/r_crit)*(r/r_crit))-1.0);
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
