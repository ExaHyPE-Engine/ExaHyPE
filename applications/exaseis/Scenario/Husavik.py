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

    domain_offset = [-48.1, 0.0, -48.2]
    domain_size = [96.4, 96.4, 96.4]

    end_time = 25.0

    # The tracer positions come from the ExaHyPE 1 setup of Leonhard Rannabauer with 241 cells along
    # each axis. They can deviate slightly from the positions of the stations.
    tracer_coordinates = [
        [9.376232362467874, 0.0001, 33.85099388956314],  # body6.75st33.85dp0.09
        [8.246753305766788, 0.0001, 11.638967936983136],  # body5.48st11.64dp0.03
        [0.7195568229321125, 0.0001, 17.21881025466928],  # body-1.53st17.22dp-0.00
        [6.327888926556476, 0.0001, 25.284520456084465],  # body3.82st25.28dp0.07
        [0.6536504638487628, 0.0001, 8.417249776348854],  # body-1.13st8.42dp-0.19
        [14.164660762183253, 0.0001, 15.356787444428392],  # body10.95st15.36dp0.10
        [1.3017916972263137, 0.0001, 15.034320297763315],  # body-1.05st15.03dp-0.00
        [25.435497238874177, 0.0001, 15.401601578655573],  # body22.92st15.40dp0.16
        [17.176126224582518, 0.0001, 7.010609949850732],  # body15.19st7.01dp-0.07
        [-4.492292804805409, 0.0001, 2.131659303817287],  # body-4.98st2.13dp-0.64
        [0.6555189792723084, 0.0001, -12.686892531023204],  # body1.98st-12.69dp-0.04
        [26.253429292802664, 0.0001, -6.9879120653472455],  # body27.40st-6.99dp0.04
        [24.94953750613164, 0.0001, -20.029540641285166],  # body27.62st-20.03dp-0.74
        [0.8540967046388133, 0.0001, -13.184495969619547],  # body2.34st-13.18dp-0.06
        [5.621345523615631, 0.0001, -9.743473279568661],  # body6.72st-9.74dp-0.05
        [-1.6476579634324366, 0.0001, -19.484646795241588],  # body0.57st-19.48dp0.06
        [17.766222982618473, 0.0001, -28.1068295777377],  # body21.14st-28.11dp-0.91
        [25.39664055692495, 0.0001, -33.966402224667945],  # body28.13st-33.97dp-0.37
        [28.08399500705491, 0.0001, -24.61055416454974],  # body30.73st-24.61dp-0.55
        [36.51336697284584, 0.0001, -42.29182654225224],  # body38.37st-42.29dp-0.13
        [
            9.376232362467874,
            -0.08808115836556851,
            33.85099388956314,
        ],  # body6.75st33.85dp0.00
        [
            8.246753305766788,
            -0.027295721133612005,
            11.638967936983136,
        ],  # body5.48st11.64dp0.00
        [
            0.7195568229321125,
            0.0011934993109008474,
            17.21881025466928,
        ],  # body-1.53st17.22dp0.00
        [
            6.327888926556476,
            -0.06958613567893224,
            25.284520456084465,
        ],  # body3.82st25.28dp0.00
        [
            0.6536504638487628,
            0.18714757066801724,
            8.417249776348854,
        ],  # body-1.13st8.42dp0.00.
        [
            14.164660762183253,
            -0.09536041761818126,
            15.356787444428392,
        ],  # body10.95st15.36dp0.00
        [
            1.3017916972263137,
            0.0029445021370122504,
            15.034320297763315,
        ],  # body-1.05st15.03dp0.00
        [
            25.435497238874177,
            -0.1627025699401088,
            15.401601578655573,
        ],  # body22.92st15.40dp0.00
        [
            17.176126224582518,
            0.0665791035924741,
            7.010609949850732,
        ],  # body15.19st7.01dp0.00
        [
            -4.492292804805409,
            0.6364933043727774,
            2.131659303817287,
        ],  # body-4.98st2.13dp0.00
        [
            0.6555189792723084,
            0.0353022881074736,
            -12.686892531023204,
        ],  # body1.98st-12.69dp0.00
        [
            26.253429292802664,
            -0.03509966432142164,
            -6.9879120653472455,
        ],  # body27.40st-6.99dp0.00
        [
            24.94953750613164,
            0.7298889865128331,
            -20.029540641285166,
        ],  # body27.62st-20.03dp0.00
        [
            0.8540967046388133,
            0.05651829134625075,
            -13.184495969619547,
        ],  # body2.34st-13.18dp0.00
        [
            5.621345523615631,
            0.0534422072164773,
            -9.743473279568661,
        ],  # body6.72st-9.74dp0.00
        [
            -1.6476579634324366,
            -0.06441300382012116,
            -19.484646795241588,
        ],  # body0.57st-19.48dp0.00
        [
            17.766222982618473,
            0.8969251457533431,
            -28.1068295777377,
        ],  # body21.14st-28.11dp0.00
        [
            25.39664055692495,
            0.3685598288835763,
            -33.966402224667945,
        ],  # body28.13st-33.97dp0.00
        [
            28.08399500705491,
            0.5510383744940981,
            -24.61055416454974,
        ],  # body30.73st-24.61dp0.00
        [
            36.51336697284584,
            0.12614619980480768,
            -42.29182654225224,
        ],  # body38.37st-42.29dp0.00
    ]

    fault_coordinates = []

    def domain_perturbations(self):
        return """
  faultNormal   = 0;
  faultPosition = _curvi->split(
    faultNormal,
    14.7,
    [](const tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler<2> faultSampler(
        {
          {"perturbation", "husavik-fault.nc"}
        },
        {112.823, 111.706},
        {-56.406, -1.106}
      );
      // The perturbation moves the plane x = 14.7 onto the fault surface of the file.
      return faultSampler.samplePiecewiseConstant("perturbation", {x[2], x[1]});
    }
  );

  _curvi->applyPerturbation(
    1,
    [](const tarch::la::Vector<DIMENSIONS, double> x) {
      static tarch::reader::NetCDFFieldSampler<2> topographySampler(
        {
          {"perturbation", "husavik-topography.nc"}
        },
        {DomainSize[0], DomainSize[2]},
        {DomainOffset[0], DomainOffset[2]}
      );
      return topographySampler.samplePiecewiseConstant("perturbation", {x[0], x[2]});
    }
  );
"""

    def initial_conditions(self):
        return """
      // The NetCDF file sets the material down to a depth of 14.875 km. A node without a positive
      // density in the file interpolates the tabulated layers linearly. The file uses the coordinate
      // transformation of the initial stress.
      static tarch::reader::NetCDFFieldSampler<3> materialSampler(
        {
          {"rho", "husavik-material.nc", "rho", {"x", "y", "z"}},
          {"cp", "husavik-material.nc", "cp", {"x", "y", "z"}},
          {"cs", "husavik-material.nc", "cs", {"x", "y", "z"}}
        },
        {(746984.1 - 563984.1), (7472491 - 7234491), (14875 + 1625.)},
        {563984.1, 7234491, -1625.}
      );

      const double easting  = 634870.90025815 + (500 * x[0] - 866.02540378 * x[2]);
      const double northing = 7327057.0394108 + (866.02540378 * x[0] + 500 * x[2]);
      const double depth    = 1000. * x[1];

      Q.rho = materialSampler.samplePiecewiseConstant("rho", {easting, northing, depth});
      Q.cp  = materialSampler.samplePiecewiseConstant("cp", {easting, northing, depth});
      Q.cs  = materialSampler.samplePiecewiseConstant("cs", {easting, northing, depth});
      if (Q.rho > 0.0) {
        // The NetCDF files cover every point with a positive density.
        return;
      }

      constexpr int    NumberOfLayers                      = 11;
      constexpr double LayerDepths[NumberOfLayers]         = {-3.0, 0.5, 2.5, 5.0, 7.0, 11., 20., 28., 32., 60., 200.};
      constexpr double Densities[NumberOfLayers]           = {2.5, 2.5, 2.7, 2.8, 2.85, 2.95, 2.95, 3.1, 3.15, 3.26, 3.33};
      constexpr double PressureWaveSpeeds[NumberOfLayers]  = {3.8, 3.8, 5.2, 6.5, 6.8, 7.0, 7.09, 7.25, 7.4, 7.7, 8.0};
      constexpr double ShearWaveSpeeds[NumberOfLayers] = {
        2.159073875530895,
        2.159073875530895,
        2.95453198650129,
        3.6931887112822785,
        3.8636406792357922,
        3.9772659035820834,
        4.028416015411062,
        4.119309368986427,
        4.204532701513361,
        4.374984662549803,
        4.45
      };
      ASSERTION(x[1] <= LayerDepths[NumberOfLayers - 1], x[1]);

      int layer = 1;
      while (x[1] > LayerDepths[layer]) {
        layer++;
      }

      const double weight = (x[1] - LayerDepths[layer - 1]) / (LayerDepths[layer] - LayerDepths[layer - 1]);
      Q.rho = Densities[layer - 1] + weight * (Densities[layer] - Densities[layer - 1]);
      Q.cp  = PressureWaveSpeeds[layer - 1] + weight * (PressureWaveSpeeds[layer] - PressureWaveSpeeds[layer - 1]);
      Q.cs  = ShearWaveSpeeds[layer - 1] + weight * (ShearWaveSpeeds[layer] - ShearWaveSpeeds[layer - 1]);
"""

    def generate_required_files(self):
        super().generate_required_files()

        for file in [
            "husavik-material.nc",
            "husavik-fault.nc",
            "husavik-topography.nc",
        ]:
            self.copy_file_to_current_folder(
                os.path.dirname(os.path.realpath(__file__)) + "/specs/Husavik/" + file,
                file,
            )

        return

    # TODO: Translate the data from the yaml files to here, and ascertain what directions any of it is in
    dynamicRuptureDictionary = {
        "initialStresses": """
  // The Andersonian stress follows Aochi and Madariaga (2003), equation A4, in the SeisSol frame with
  // x pointing east, y north and z up. The vertical stress is the intermediate principal stress. A
  // negative stress compresses.
  constexpr double StaticFriction          = 0.55;
  constexpr double DynamicFriction         = 0.1;
  constexpr double Cohesion                = 0.;
  constexpr double RelativePrestress       = 0.55;
  // An intermediate stress ratio below 0.5 induces transpression, above 0.5 transtension.
  constexpr double IntermediateStressRatio = 0.5;
  // The maximum horizontal stress points 30 degrees off the strike of the fault.
  constexpr double MaximumHorizontalStressAzimuth = 140.;

  const double height                   = -1000. * x[1];
  const double effectiveVerticalStress  = 2670.0 * 0.4 * 9.8 * std::min(-1500.0, height);
  const double effectiveConfiningStress = std::fabs(effectiveVerticalStress);

  const double optimalFaultAngle  = M_PI / 4.0 - 0.5 * std::atan(StaticFriction);
  const double sineOfTwiceAngle   = std::sin(2.0 * optimalFaultAngle);
  const double cosineOfTwiceAngle = std::cos(2.0 * optimalFaultAngle);
  const double shapeFactor        = (2.0 * IntermediateStressRatio - 1.0) / 3.0;
  const double stressDifference =
    (DynamicFriction * effectiveConfiningStress
     + RelativePrestress * (Cohesion + (StaticFriction - DynamicFriction) * effectiveConfiningStress))
    / (sineOfTwiceAngle + DynamicFriction * (shapeFactor + cosineOfTwiceAngle)
       + RelativePrestress * (StaticFriction - DynamicFriction) * (shapeFactor + cosineOfTwiceAngle));
  const double meanStress              = effectiveConfiningStress - shapeFactor * stressDifference;
  const double maximumHorizontalStress = meanStress + stressDifference;
  const double minimumHorizontalStress = meanStress - stressDifference;
  const double verticalStress = meanStress - stressDifference + 2.0 * stressDifference * IntermediateStressRatio;

  const double azimuth       = MaximumHorizontalStressAzimuth * M_PI / 180.0;
  const double azimuthCosine = std::cos(azimuth);
  const double azimuthSine   = std::sin(azimuth);
  const double andersonianXX =
    -(azimuthCosine * azimuthCosine * minimumHorizontalStress + azimuthSine * azimuthSine * maximumHorizontalStress)
    * effectiveConfiningStress / verticalStress;
  const double andersonianXY = -azimuthCosine * azimuthSine * (maximumHorizontalStress - minimumHorizontalStress)
                             * effectiveConfiningStress / verticalStress;
  const double andersonianYY =
    -(azimuthSine * azimuthSine * minimumHorizontalStress + azimuthCosine * azimuthCosine * maximumHorizontalStress)
    * effectiveConfiningStress / verticalStress;
  const double andersonianZZ = -effectiveConfiningStress;

  // The stress tapers from the Andersonian stress to the confining stress between the heights
  // TaperStart and TaperStop.
  constexpr double TaperStart = -9000.;
  constexpr double TaperStop  = -11000.;
  constexpr double TaperWidth = (TaperStart - TaperStop);
  double           taper      = 0.001;
  if (height >= TaperStart) {
    taper = 1.0;
  } else if (height >= TaperStop) {
    const double position = 1.0 - (height - TaperStop) / TaperWidth;
    taper                 = 1.0 - (3.0 * position * position - 2.0 * position * position * position);
  }

  const double stressXX = taper * andersonianXX + (1.0 - taper) * effectiveVerticalStress;
  const double stressYY = taper * andersonianYY + (1.0 - taper) * effectiveVerticalStress;
  const double stressZZ = taper * andersonianZZ + (1.0 - taper) * effectiveVerticalStress;
  const double stressXY = taper * andersonianXY;

  // The rotation into the mesh frame converts the stress from Pa to MPa.
  sxx = 0.000001 * (0.25 * stressXX + 0.8660254 * stressXY + 0.75 * stressYY);
  sxy = 0;
  sxz = 0.000001 * (-0.4330127 * stressXX - 0.5 * stressXY + 0.4330127 * stressYY);
  syy = 0.000001 * (stressZZ);
  syz = 0;
  szz = 0.000001 * (0.75 * stressXX - 0.8660254 * stressXY + 0.25 * stressYY);
    """,
        "n_yz": "return 0;",
        "cohesion": """
  constexpr double BaseCohesion           = -.4;
  constexpr double CohesionIncreaseHeight = 10.0;
  const double     height                 = -x[1];
  if (height >= -CohesionIncreaseHeight) {
    return -(BaseCohesion - 2.0 * (height + CohesionIncreaseHeight) / CohesionIncreaseHeight);
  }
  return -BaseCohesion;
""",
        "forced_rupture_time": """
  const double     easting          = 634870.90025815 + (500 * x[0] - 866.02540378 * x[2]);
  const double     northing         = 7327057.0394108 + (866.02540378 * x[0] + 500 * x[2]);
  const double     height           = -1000. * x[1];
  const double     distance         = std::sqrt(
    std::pow(easting - 639613.9, 2.0) + std::pow(northing - 7340258.5, 2.0) + std::pow(height + 5000.0, 2.0)
  );
  constexpr double CriticalDistance = 3000.0;
  constexpr double ShearWaveSpeed   = 3800.0;
  if (distance <= CriticalDistance) {
    return distance / (0.7 * ShearWaveSpeed)
         + (0.081 * CriticalDistance / (0.7 * ShearWaveSpeed))
             * (1.0 / (1.0 - std::pow(distance / CriticalDistance, 2.0)) - 1.0);
  }
  return 1000000000.0;
""",
        "d_c": "return 0.5;",
        # constant parameters
        "mu_d": 0.1,
        "mu_s": 0.55,
        "t0_rupture": 0.5,
        "f_cy": 0,
        "f_wy": 13.5,
        "f_cz": 0.1619223838720245,
        "f_wz": 42.20463397793492,
    }
