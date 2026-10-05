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
    doi.org/10.1007/978-3-030-43229-4_27. The note on mapQuantities() explains the visualisation.
    """

    _dimensions = 2
    _offset = 0.0
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
        Gravity * std::pow(ReferencePressure, GasConstant / IsobaricHeatCapacity) * x[DIMENSIONS-1] /
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

  std::fill_n(Q, NumberOfUnknowns+NumberOfAuxiliaryVariables, 0);

  // The state with the perturbation of the potential temperature. """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto temperature = potentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity
  );

  const auto rho = pressure / (GasConstant * temperature);
  const auto height = x[DIMENSIONS-1];
  Q[Shortcuts::rho] = rho;
  Q[Shortcuts::height] = height;

  const auto* const j = &Q[Shortcuts::j];
  const auto Z = perturbation;
  """
            + self._equation.evaluate_energy()
            + """

  Q[Shortcuts::E] = E;

  // The background state without the perturbation of the potential temperature.
  const auto backgroundTemperature = BackgroundPotentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity
  );
  const auto backgroundRho = pressure / (GasConstant * backgroundTemperature);
  Q[Shortcuts::backgroundstate+0] = backgroundRho;
  Q[Shortcuts::backgroundstate+1] = pressure;
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

  for (int value = 0; value < NumberOfUnknowns + NumberOfAuxiliaryVariables; value++) {
    Qoutside[value] = Qinside[value];
  }

  // The mirrored normal momentum gives a zero normal velocity at the wall.
  Qoutside[Shortcuts::j+normal] = -Qinside[Shortcuts::j+normal];

  // The hydrostatic background state. """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto temperature = BackgroundPotentialTemperature *
    std::pow((pressure / ReferencePressure), GasConstant / IsobaricHeatCapacity);

  const auto rho = pressure / (GasConstant * temperature);
  const auto height = Qoutside[Shortcuts::height];
  const auto* const j = &Qoutside[Shortcuts::j];

  Qoutside[Shortcuts::rho] = rho;
  Qoutside[Shortcuts::backgroundstate+0] = Qoutside[Shortcuts::rho];
  Qoutside[Shortcuts::backgroundstate+1] = pressure;

  const auto Z = 0;
  """
            + self._equation.evaluate_energy()
            + """
  Qoutside[Shortcuts::E] = E;
"""
        )

    # Returns the C++ code of a mapping from the conserved variables to the density, the velocities,
    # the pressure and the potential temperature. The potential temperature shows the bubbles, whose
    # variation in the conserved variables is small against the reference pressure.
    def mapQuantities(self):
        return (
            """
void tests::exahype2::aderdg::ADERDGSolver::mapQuantities(
  [[maybe_unused]] const double* const NOALIAS  Q, // Q[4+3]
  [[maybe_unused]] double* const NOALIAS  outputQuantities // Q[4+3]
){
  constexpr auto AdiabaticIndex = 1.4;
  constexpr auto GasConstant = 287.058;
  constexpr auto IsobaricHeatCapacity = AdiabaticIndex / (AdiabaticIndex - 1) * GasConstant;
  constexpr auto ReferencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto Gravity = 9.81;
  constexpr auto HeatRelease = 0;

  const auto* const j = &Q[Shortcuts::j];
  const auto E = Q[Shortcuts::E];
  const auto rho = Q[Shortcuts::rho];
  const auto height = Q[Shortcuts::height];

  const auto Z = 0;
  """
            + self._equation.evaluate_pressure()
            + """

  outputQuantities[Shortcuts::rho] = rho;
  outputQuantities[Shortcuts::j+0] = Q[Shortcuts::j+0]/rho;
  outputQuantities[Shortcuts::j+1] = Q[Shortcuts::j+1]/rho;
#if DIMENSIONS == 3
  outputQuantities[Shortcuts::j+2] = Q[Shortcuts::j+2]/rho;
#endif
  outputQuantities[Shortcuts::E] = pressure;

  const auto temperature = pressure/(GasConstant * rho);
  const auto potentialTemperature = temperature / std::pow((pressure / ReferencePressure), (GasConstant / IsobaricHeatCapacity));

  outputQuantities[Shortcuts::height] = potentialTemperature;
}
"""
        )


class NavierStokesCosineBubble(NavierStokesBubbles):
    """
    A single warm bubble in a homogeneous domain, which rises and diffuses outward. The potential
    temperature shows the bubble best.
    """

    def evaluate_perturbation(self):
        return """
  const tarch::la::Vector<DIMENSIONS, double> bubbleCentre = {
    500, 350
#if DIMENSIONS == 3
    ,350
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
    500, 300
#if DIMENSIONS == 3
    ,300
#endif
  };

  const tarch::la::Vector<DIMENSIONS, double> coldBubbleCentre = {
    560, 640
#if DIMENSIONS == 3
    ,640
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
