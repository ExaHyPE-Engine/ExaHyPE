# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DynamicRuptureScenario import DynamicRuptureScenario

import os

class Husavik(DynamicRuptureScenario):
    """
    Simulates an earthquake in the north Iceland region, around the Húsavı́k-Flatey fault.
    The scenario was originally implemented by Leonhard Rannabauer, and most of the files
    can be found in his zenodo repository at https://zenodo.org/records/6386282, except for
    the material parameters which are missing.
    Similar results should be reproduceable from https://zenodo.org/records/8021690 and
    https://zenodo.org/records/7990543, with the later being where we recovered the
    material parameters from.

    The description of the scenario and results be found in Leonhard Rannabauer's thesis,
      in section 8.5: https://mediatum.ub.tum.de/node?id=1639212
    """

    domain_offset   = [-48.1, 0.0, -48.2]
    domain_size     = [96.4, 96.4, 96.4]

    end_time = 25.0

    #These tracers were simply copied from Leonhard Rannabauer's work, where they were hardcoded
    # for the ExaHyPE1 case where the domain was divided into 241 cells, as such they are likely
    # to be slightly off from the correct position in all cases
    tracer_coordinates = [
      [9.376232362467874,  0.0001, 33.85099388956314],  #body6.75st33.85dp0.09
      [8.246753305766788,  0.0001, 11.638967936983136], #body5.48st11.64dp0.03
      [0.7195568229321125, 0.0001, 17.21881025466928],  #body-1.53st17.22dp-0.00
      [6.327888926556476,  0.0001, 25.284520456084465], #body3.82st25.28dp0.07
      [0.6536504638487628, 0.0001, 8.417249776348854],  #body-1.13st8.42dp-0.19

      [14.164660762183253, 0.0001, 15.356787444428392], #body10.95st15.36dp0.10
      [1.3017916972263137, 0.0001, 15.034320297763315], #body-1.05st15.03dp-0.00
      [25.435497238874177, 0.0001, 15.401601578655573], #body22.92st15.40dp0.16
      [17.176126224582518, 0.0001, 7.010609949850732],  #body15.19st7.01dp-0.07
      [-4.492292804805409, 0.0001, 2.131659303817287],  #body-4.98st2.13dp-0.64

      [0.6555189792723084, 0.0001, -12.686892531023204], #body1.98st-12.69dp-0.04
      [26.253429292802664, 0.0001, -6.9879120653472455], #body27.40st-6.99dp0.04
      [24.94953750613164,  0.0001, -20.029540641285166], #body27.62st-20.03dp-0.74
      [0.8540967046388133, 0.0001, -13.184495969619547], #body2.34st-13.18dp-0.06
      [5.621345523615631,  0.0001, -9.743473279568661],  #body6.72st-9.74dp-0.05

      [-1.6476579634324366, 0.0001, -19.484646795241588], #body0.57st-19.48dp0.06
      [17.766222982618473,  0.0001, -28.1068295777377],   #body21.14st-28.11dp-0.91
      [25.39664055692495,   0.0001, -33.966402224667945], #body28.13st-33.97dp-0.37
      [28.08399500705491,   0.0001, -24.61055416454974],  #body30.73st-24.61dp-0.55
      [36.51336697284584,   0.0001, -42.29182654225224],  #body38.37st-42.29dp-0.13

      [9.376232362467874, -0.08808115836556851,   33.85099388956314],   #body6.75st33.85dp0.00
      [8.246753305766788, -0.027295721133612005,  11.638967936983136],  #body5.48st11.64dp0.00
      [0.7195568229321125, 0.0011934993109008474, 17.21881025466928],   #body-1.53st17.22dp0.00
      [6.327888926556476, -0.06958613567893224,   25.284520456084465],  #body3.82st25.28dp0.00
      [0.6536504638487628, 0.18714757066801724,   8.417249776348854],   #body-1.13st8.42dp0.00.

      [14.164660762183253, -0.09536041761818126,    15.356787444428392],  #body10.95st15.36dp0.00
      [1.3017916972263137,  0.0029445021370122504,  15.034320297763315],  #body-1.05st15.03dp0.00
      [25.435497238874177, -0.1627025699401088,     15.401601578655573],  #body22.92st15.40dp0.00
      [17.176126224582518,  0.0665791035924741,     7.010609949850732],   #body15.19st7.01dp0.00
      [-4.492292804805409,  0.6364933043727774,     2.131659303817287],   #body-4.98st2.13dp0.00

      [0.6555189792723084,  0.0353022881074736,   -12.686892531023204], #body1.98st-12.69dp0.00
      [26.253429292802664, -0.03509966432142164,  -6.9879120653472455], #body27.40st-6.99dp0.00
      [24.94953750613164,   0.7298889865128331,   -20.029540641285166], #body27.62st-20.03dp0.00
      [0.8540967046388133,  0.05651829134625075,  -13.184495969619547], #body2.34st-13.18dp0.00
      [5.621345523615631,   0.0534422072164773,   -9.743473279568661],  #body6.72st-9.74dp0.00

      [-1.6476579634324366, -0.06441300382012116, -19.484646795241588], #body0.57st-19.48dp0.00
      [17.766222982618473,   0.8969251457533431,  -28.1068295777377],   #body21.14st-28.11dp0.00
      [25.39664055692495,    0.3685598288835763,  -33.966402224667945], #body28.13st-33.97dp0.00
      [28.08399500705491,    0.5510383744940981,  -24.61055416454974],  #body30.73st-24.61dp0.00
      [36.51336697284584,    0.12614619980480768, -42.29182654225224]   #body38.37st-42.29dp0.00
    ]

    fault_coordinates = [
    ]

    def domain_perturbations(self):
        return """
  _curvi->applyPerturbation(
    1,
    [](tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler<2> netcdfSampler(
        {
          {"perturbation", "husavik-topography.nc"}
        },
        {DomainSize[0], DomainSize[2]},
        {DomainOffset[0], DomainOffset[2]}
      );
      return netcdfSampler.sample(
        "perturbation",
        {x[0], x[2]}
      );
    }
  );

  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    14.7,
    [](tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler<2> netcdfSampler(
        {
          {"perturbation", "husavik-fault.nc"}
        },
        {112.823, 111.706},
        {-56.406, -1.106}
      );
      return netcdfSampler.sample(
        "perturbation",
        {x[2], x[1]}
      );
    }
  );
"""

    def initial_conditions(self):
        return """
        // Two criteria: under a given depth (about 14.875km) we use values from a file,
        // beneath that depth we interpolate linearly from pre-computed values
        // note that the netcdf file uses the same domain transformation as the stresses, see beneath
        static tarch::reader::NetCDFFieldSampler<3> materialSampler(
          {
            {"rho", "husavik-material.nc", "rho", {"x", "y", "z"}},
            {"cp",  "husavik-material.nc", "cp",  {"x", "y", "z"}},
            {"cs",  "husavik-material.nc", "cs",  {"x", "y", "z"}}
          },
          {
            (746984.1-563984.1),
            (7472491-7234491),
            (14875+1625.)
          },
          {
            563984.1,
            7234491,
            -1625.
          }
        );

        const double xx = 634870.90025815+(500*x[0]-866.02540378*x[2]);
        const double y  = 7327057.0394108+(866.02540378*x[0]+500*x[2]);
        const double z  = 1000.*x[1];

        Q[Shortcuts::rho] = materialSampler.sample(
          "rho", {xx, y, z}
        );
        Q[Shortcuts::cp] = materialSampler.sample(
          "cp", {xx, y, z}
        );
        Q[Shortcuts::cs] = materialSampler.sample(
          "cs", {xx, y, z}
        );

        if(Q[Shortcuts::rho]>0.){
          //We got values from the netcdf files, so no need to use the linear interpolation
          return;
        }

        constexpr double xPos[11] = {
          -3.0, 0.5, 2.5, 5.0, 7.0, 11.,
          20., 28., 32., 60., 200.
        };
        constexpr double rho[11] = {
          2.5, 2.5, 2.7, 2.8, 2.85, 2.95,
          2.95, 3.1, 3.15, 3.26, 3.33
        };
        constexpr double cp[11] = {
          3.8, 3.8, 5.2, 6.5, 6.8, 7.0,
          7.09, 7.25, 7.4, 7.7, 8.0
        };
        constexpr double cs[11] = {
          2.159073875530895,  2.159073875530895,
          2.95453198650129,   3.6931887112822785,
          3.8636406792357922, 3.9772659035820834,
          4.028416015411062,  4.119309368986427,
          4.204532701513361,  4.374984662549803,
          4.45
        };

        int i=1;
        while(x[1]>xPos[i]){
          i++;
        }

        // y = y0 + (x-x0) * (y1-y0) / (x1-x0);
        const double xdiff = (x[1]-xPos[i-1])/(xPos[i]-xPos[i-1]);
        const double rhoL = rho[i-1];
        const double rhoR = rho[i];
        Q[Shortcuts::rho] = rhoL+xdiff*(rhoR-rhoL);
        const double cpL = cp[i-1];
        const double cpR = cp[i];
        Q[Shortcuts::cp] = cpL+xdiff*(cpR-cpL);
        const double csL = cs[i-1];
        const double csR = cs[i];
        Q[Shortcuts::cs] = csL+xdiff*(csR-csL);
"""

    def generate_required_files(self):
        super().generate_required_files()

        for file in ["husavik-material.nc", "husavik-fault.nc", "husavik-topography.nc"]:
          self.copy_file_to_current_folder(
              os.path.dirname(os.path.realpath(__file__))+"/specs/Husavik/" + file,
              file
          )

        return

    # TODO: translate the data from the yaml files to here, and ascertain what directions any of it is in
    dynamicRuptureDictionary = {
    "initialStresses": """
    auto evaluateAndersonianStress = [&](
      const double* const x,
      double& b_xx,
      double& b_yy,
      double& b_zz,
      double& b_xy,
      double& b_yz,
      double& b_xz
    ) -> void {

      constexpr double mu_s = 0.55;
      constexpr double mu_d = 0.1;
      // strike alpha means Shmax=alpha+30
      constexpr double SH_max = 140.;
      constexpr double cohesion = 0.;
      // s2ratio<0.5 induces transpression, else transtension
      constexpr double s2ratio = 0.5;
      constexpr double i_S_v = 2;

      constexpr double R = 0.55;
      // constexpr double S = (1.0/R-1.0);

      const double z  = -1000.*x[1];
      const double sig_zz = 2670.0*0.4*9.8*std::min(-1500.0,z);

      // most favorable direction (A4, AM2003)
      double Phi = std::numbers::pi / 4.0 - 0.5 * atan(mu_s);
      double SHmax_rad = SH_max * std::numbers::pi / 180.0;
      double s2 = sin(2.0 * Phi);
      double c2 = cos(2.0 * Phi);
      double alpha = (2.0 * s2ratio - 1.0) / 3.0;
      // we assume that szz is the effectiveConfiningStress  = sii/3 which is not necessarily true,
      // but then we scale the stress to get b_zz = szz
      double effectiveConfiningStress = std::fabs(sig_zz);
      // double R = 1.0 / (1.0 + i.S);
      double ds = (mu_d * effectiveConfiningStress +
                  R * (cohesion + (mu_s - mu_d) * effectiveConfiningStress)) /
                  (s2 + mu_d * (alpha + c2) + R * (mu_s - mu_d) * (alpha + c2));
      double sm = effectiveConfiningStress - alpha * ds;
      // sii are all positive
      double s11 = sm + ds;
      double s22 = sm - ds + 2.0 * ds * s2ratio;
      double s33 = sm - ds;
      double S_hmax, S_hmin, S_v;
      if (i_S_v == 1) {
          S_hmax = s22;
          S_hmin = s33;
          S_v = s11;
      } else {
          if (i_S_v == 2) {
              S_hmax = s11;
              S_hmin = s33;
              S_v = s22;
          } else {
              S_hmax = s11;
              S_hmin = s22;
              S_v = s33;
          }
      }

      double cs = cos(SHmax_rad);
      double ss = sin(SHmax_rad);

      // minus signs added to fall back to SeisSol convention (compressive stress<0)

      b_xx = -(cs * cs * S_hmin + ss * ss * S_hmax) * effectiveConfiningStress / S_v;
      b_xy = -cs * ss * (S_hmax - S_hmin) * effectiveConfiningStress / S_v;
      b_xz = 0;
      b_yy = -(ss * ss * S_hmin + cs * cs * S_hmax) * effectiveConfiningStress / S_v;
      b_yz = 0;
      b_zz = -effectiveConfiningStress;

    };

    double b_xx, b_yy, b_zz,
        b_xy, b_xz, b_yz;

    evaluateAndersonianStress(
        x,
        b_xx,
        b_yy,
        b_zz,
        b_xy,
        b_yz,
        b_xz
    );

    const double z  = -1000.*x[1];

    constexpr double zStressDecreaseStart = -9000.;
    constexpr double zStressDecreaseStop = -11000.;
    constexpr double zStressDecreaseWidth = (zStressDecreaseStart - zStressDecreaseStop);
    double Omega;
    if  (z>=zStressDecreaseStart) {
      Omega = 1.0;
    } else {
      if (z>=zStressDecreaseStop) {
          const double a = 1.0-(z-zStressDecreaseStop)/zStressDecreaseWidth;
          const double Sx = (3.0*a*a-2.0*a*a*a);
          Omega = 1.0-Sx;
      } else {
        Omega = 0.001;
      }
    }

    const double eCS = 2670.0*0.4*9.8*std::min(-1500.0,z);

    const double stresses[6] = {
        Omega*b_xx + (1.0-Omega)*eCS, //s_xx
        Omega*b_yy + (1.0-Omega)*eCS, //s_yy
        Omega*b_zz + (1.0-Omega)*eCS, //s_zz
        Omega*b_xy, //s_xy
        Omega*b_yz, //s_yz
        Omega*b_xz  //s_xz
    };

    sxx = 0.000001*(0.25*stresses[0]+0.8660254*stresses[3]+0.75*stresses[1]);
    sxy = 0.000001*(0.5*stresses[5]+0.8660254*stresses[4]);
    sxz = 0.000001*(-0.4330127*stresses[0]-0.5*stresses[3]+0.4330127*stresses[1]);
    syy = 0.000001*(stresses[2]);
    syz = 0.000001*(-0.8660254*stresses[5]+0.5*stresses[4]);
    szz = 0.000001*(0.75*stresses[0]-0.8660254*stresses[3]+0.25*stresses[1]);
    """,
      "n_yz": "return 0;",
      "cohesion": """
  constexpr double cohesion = -.4;
  constexpr double zIncreasingCohesion = 10.0;
  const double z = -x[1];
  if (z >= -zIncreasingCohesion) {
    return -(cohesion - 2.0 * (z+zIncreasingCohesion)/zIncreasingCohesion);
  }
  return -cohesion;
""",
      "forced_rupture_time": """
  const double xx = 634870.90025815+(500*x[0]-866.02540378*x[2]);
  const double y  = 7327057.0394108+(866.02540378*x[0]+500*x[2]);
  const double z  = -1000.*x[1];
  const double r = sqrt(pow(xx-639613.9, 2.0) + pow(y-7340258.5, 2.0) + pow(z+5000.0, 2.0));
  constexpr double r_crit = 3000.0;
  constexpr double Vs = 3800.0;
  if (r <= r_crit) {
    return r/(0.7*Vs)+(0.081*r_crit/(0.7*Vs))*(1.0/(1.0-pow(r/r_crit, 2.0))-1.0);
  }
  return 1000000000.0;
""",
      "d_c": "return 0.5;",
  #constant parameters
      "mu_d": 0.1,
      "mu_s": 0.55,
      "t0_rupture": 0.5,
      "f_cy": 0,
      "f_wy": 13.5,
      "f_cz": 0.1619223838720245,
      "f_wz": 42.20463397793492
    }
