# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import NavierStokes


class NavierStokesBubbles(Scenario):
    """
    Cloud formations of the compressible Navier-Stokes equations after
    doi.org/10.1007/978-3-030-43229-4_27.
    """

    _dimensions = 2
    _offset = -500.0
    _domain_size = 1000.0
    _periodic_bc = False
    _end_time = 100

    # The heat capacity at constant volume is gas_constant / (gamma - 1).
    cv = 1 / (1.4 - 1) * 287.058
    _equation = NavierStokes(
        dimensions=2,
        use_advection=False,
        use_background_state=True,
        use_gravity=True,
        gamma=1.4,
        cv=cv,
        gas_constant=287.058,
        reference_viscosity=0.01,
        prandtl_number=0.71,
    )

    _reference_pressure = 100000.0
    _background_potential_temperature = 300.0

    def __init__(self):
        return

    def evaluate_perturbation(self):
        raise NotImplementedError("A subclass defines the perturbation.")

    def evaluate_hydrostatic_pressure(self):
        return """
  const auto pressure = std::pow(
    (1 - 1 / AdiabaticIndex) *
        (std::pow(std::pow(AdiabaticIndex, AdiabaticIndex / (AdiabaticIndex - 1)) *
                      ReferencePressure,
                  (AdiabaticIndex - 1) / AdiabaticIndex) /
            (AdiabaticIndex - 1) -
        Gravity * std::pow(ReferencePressure, GasConstant / IsobaricHeatCapacity) *
            (x[DIMENSIONS-1] - DomainOffset[DIMENSIONS-1]) /
            (GasConstant * BackgroundPotentialTemperature)),
    AdiabaticIndex / (AdiabaticIndex - 1)
  );
"""

    def initial_conditions(self):
        return (
            """

  constexpr auto AdiabaticIndex = 1.4;
  constexpr auto GasConstant = 287.058;
  constexpr auto IsobaricHeatCapacity = AdiabaticIndex / (AdiabaticIndex - 1) * GasConstant;
  constexpr auto ReferencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto BackgroundPotentialTemperature = """
            + str(self._background_potential_temperature)
            + """;
  constexpr auto Gravity = 9.81;
  constexpr auto HeatRelease = 0;

  """
            + self.evaluate_perturbation()
            + """

  const auto potentialTemperature = BackgroundPotentialTemperature + perturbation;

  Q = {};

  // The state with the perturbation of the potential temperature. """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto temperature = potentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity
  );

  const auto rho = pressure / (GasConstant * temperature);
  const auto height = x[DIMENSIONS-1] - DomainOffset[DIMENSIONS-1];
  Q.rho    = rho;
  Q.height = height;

  const auto& j = Q.j;
  const auto Z = perturbation;
  """
            + self._equation.evaluate_energy()
            + """

  Q.E = E;

  // The background state without the perturbation of the potential temperature.
  const auto backgroundTemperature = BackgroundPotentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity
  );
  const auto backgroundRho = pressure / (GasConstant * backgroundTemperature);
  Q.backgroundstate(0) = backgroundRho;
  Q.backgroundstate(1) = pressure;
"""
        )

    def boundary_conditions(self):
        return (
            """
  constexpr auto AdiabaticIndex = 1.4;
  constexpr auto GasConstant = 287.058;
  constexpr auto IsobaricHeatCapacity = AdiabaticIndex / (AdiabaticIndex - 1) * GasConstant;
  constexpr auto ReferencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto BackgroundPotentialTemperature = """
            + str(self._background_potential_temperature)
            + """;
  constexpr auto Gravity = 9.81;
  constexpr auto HeatRelease = 0;

  Qoutside = Qinside;

  // The mirrored normal momentum gives a zero normal velocity at the wall.
  Qoutside.j(normal) = -Qinside.j(normal);

  // The hydrostatic background state. """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto temperature = BackgroundPotentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity);

  const auto rho = pressure / (GasConstant * temperature);
  const auto height = Qoutside.height;
  const auto& j = Qoutside.j;

  Qoutside.rho                = rho;
  Qoutside.backgroundstate(0) = Qoutside.rho;
  Qoutside.backgroundstate(1) = pressure;

  const auto Z = 0;
  """
            + self._equation.evaluate_energy()
            + """
  Qoutside.E = E;
"""
        )


class NavierStokesCosineBubble(NavierStokesBubbles):
    """
    A single warm bubble in a homogeneous domain. The bubble rises and diffuses outward. The
    potential temperature shows the bubble best.
    """

    def evaluate_perturbation(self):
        return """
  const tarch::la::Vector<DIMENSIONS, double> bubbleCentre = {
    0, -150
#if DIMENSIONS == 3
    ,-150
#endif
  };

  constexpr double BubbleRadius = 250;
  constexpr auto TemperatureDifference = 0.5;

  const auto distance = tarch::la::norm2(x-bubbleCentre);
  const auto perturbation = distance <= BubbleRadius ?
    TemperatureDifference / 2 * (1 + std::cos((std::numbers::pi * distance) / BubbleRadius))
    : 0;
"""


class NavierStokesTwoBubbles(NavierStokesBubbles):
    """
    A warm bubble beneath a cold bubble in a homogeneous domain. The bubbles move towards each
    other, collide and mix.
    """

    def evaluate_perturbation(self):
        return """
  const tarch::la::Vector<DIMENSIONS, double> hotBubbleCentre = {
    0, -200
#if DIMENSIONS == 3
    ,-200
#endif
  };

  const tarch::la::Vector<DIMENSIONS, double> coldBubbleCentre = {
    60, 140
#if DIMENSIONS == 3
    ,140
#endif
  };

  constexpr double DecayLength               = 50.;
  constexpr double HotBubbleRadius           = 150;
  constexpr double ColdBubbleRadius          = 0;
  constexpr double HotTemperatureDifference  = 0.5;
  constexpr double ColdTemperatureDifference = -0.15;

  const auto hotBubbleDistance = tarch::la::norm2(x-hotBubbleCentre) - HotBubbleRadius;
  auto perturbation = hotBubbleDistance <= 0 ? HotTemperatureDifference
    : HotTemperatureDifference * std::exp(-(hotBubbleDistance * hotBubbleDistance) / (DecayLength * DecayLength));

  const auto coldBubbleDistance = tarch::la::norm2(x-coldBubbleCentre) - ColdBubbleRadius;
  perturbation += ColdTemperatureDifference * std::exp(-(coldBubbleDistance * coldBubbleDistance) / (DecayLength * DecayLength));
"""
