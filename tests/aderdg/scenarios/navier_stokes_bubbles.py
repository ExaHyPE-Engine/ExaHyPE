# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import NavierStokes


class NavierStokesBubbles(Scenario):
    """
    Scenarios described in doi.org/10.1007/978-3-030-43229-4_27.

    Simulation of cloud formations via the compressible Navier-Stokes equations.
    See the function 'mapQuantities' below for a note on the visualisation.
    """

    _dimensions = 2
    _offset = 0.0
    _domain_size = 1000.0
    _periodic_bc = False
    # _end_time = 600
    _end_time = 100

    # cv = 1 / (gamma - 1) * gasConstant
    cv = 1 / (1.4 - 1) * 287.058
    _equation = NavierStokes(
        dimensions=2,
        use_advection=False,
        use_background_state=True,
        use_gravity=True,
        use_viscosity=True,
        gamma=1.4,
        cv=cv,
        gas_constant=287.058,
        reference_viscosity=0.01,
        Pr=0.71,
    )

    _reference_pressure = 100000.0
    _background_potential_temperature = 300.0

    def __init__(self):
        return

    def evaluate_perturbation(self):
        raise NotImplementedError("perturbation should be defined in child classes")

    def evaluate_hydrostatic_pressure(self):
        return """
  const auto pressure = std::pow(
    (1 - 1 / gamma) *
        (std::pow(std::pow(gamma, gamma / (gamma - 1)) *
                      referencePressure,
                  (gamma - 1) / gamma) /
            (gamma - 1) -
        gravity * std::pow(referencePressure, gasConstant / cp) * x[DIMENSIONS-1] /
            (gasConstant * backgroundPotentialTemperature)),
    gamma / (gamma - 1)
  );
"""

    def initial_conditions(self):
        return (
            """

  constexpr auto gamma = 1.4;
  constexpr auto gasConstant = 287.058;
  constexpr auto cp = gamma / (gamma - 1) * gasConstant;
  constexpr auto referencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto backgroundPotentialTemperature = """
            + str(self._background_potential_temperature)
            + """;
  constexpr auto gravity = 9.81;
  constexpr auto q0 = 0;

  """
            + self.evaluate_perturbation()
            + """

  auto potentialT = backgroundPotentialTemperature;
  potentialT += perturbation;

  std::fill_n(Q, NumberOfUnknowns+NumberOfAuxiliaryVariables, 0);

  // First compute overall state """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto temperature = potentialT *
    std::pow((pressure / referencePressure), gasConstant / cp
  );

  auto rho = pressure / (gasConstant * temperature);
  auto height = x[DIMENSIONS-1];
  Q[Shortcuts::rho] = rho;
  Q[Shortcuts::height] = height;

  auto j = &Q[Shortcuts::j];
  auto Z = perturbation;
  """
            + self._equation.evaluate_energy()
            + """

  Q[Shortcuts::E] = E;

  // Then compute background state (without pot.T. perturbation)
  const auto backgroundTemperature = backgroundPotentialTemperature *
    std::pow((pressure / referencePressure), gasConstant / cp
  );
  const auto backgroundRho = pressure / (gasConstant * backgroundTemperature);
  Q[Shortcuts::backgroundstate+0] = backgroundRho;
  Q[Shortcuts::backgroundstate+1] = pressure;
"""
        )

    def boundary_conditions(self):
        return (
            """
  constexpr auto gamma = 1.4;
  constexpr auto gasConstant = 287.058;
  constexpr auto cp = gamma / (gamma - 1) * gasConstant;
  constexpr auto referencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto backgroundPotentialTemperature = """
            + str(self._background_potential_temperature)
            + """;
  constexpr auto gravity = 9.81;
  constexpr auto q0 = 0;

  for (int value = 0; value < NumberOfUnknowns + NumberOfAuxiliaryVariables; value++) {
    Qoutside[value] = Qinside[value];
  }

  // Normal velocity zero after Riemann.
  Qoutside[Shortcuts::j+normal] = -Qinside[Shortcuts::j+normal];

  // First compute overall state """
            + (self.evaluate_hydrostatic_pressure())
            + """
  const auto T = backgroundPotentialTemperature *
    std::pow((pressure / referencePressure), gasConstant / cp);

  auto rho = pressure / (gasConstant * T);
  auto height = Qoutside[Shortcuts::height];
  auto j = &Qoutside[Shortcuts::j];

  Qoutside[Shortcuts::rho] = rho;
  Qoutside[Shortcuts::backgroundstate+0] = Qoutside[Shortcuts::rho];
  Qoutside[Shortcuts::backgroundstate+1] = pressure;

  auto Z = 0;
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
  constexpr auto gamma = 1.4;
  constexpr auto gasConstant = 287.058;
  constexpr auto cp = gamma / (gamma - 1) * gasConstant;
  constexpr auto referencePressure = """
            + str(self._reference_pressure)
            + """;
  constexpr auto gravity = 9.81;
  constexpr auto q0 = 0;

  const auto j = &Q[Shortcuts::j];
  auto E = Q[Shortcuts::E];
  auto rho = Q[Shortcuts::rho];
  auto height = Q[Shortcuts::height];

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

  const auto temperature = pressure/(gasConstant * rho);
  const auto potT = temperature / std::pow((pressure / referencePressure), (gasConstant / cp));

  outputQuantities[Shortcuts::height] = potT;
}
"""
        )


class NavierStokesCosineBubble(NavierStokesBubbles):
    """
    Here we simulate a homogeneous domain containing a single spherical bubble
    of higher temperature.
    Due to the higher temperature the bubble will rise and diffuse outward.

    The bubble is best visualised in the potential temperature
    """

    def evaluate_perturbation(self):
        return """
  // evaluate perturbation
  tarch::la::Vector<DIMENSIONS, double> bubbleCenter = {
    500, 350
#if DIMENSIONS == 3
    ,350
#endif
  };

  constexpr double size = 250;
  constexpr auto tempDifference = 0.5;

  const auto dist = tarch::la::norm2(x-bubbleCenter);
  auto perturbation = dist <= size ?
    tempDifference / 2 * (1 + std::cos((std::numbers::pi * dist) / size))
    : 0;
"""


class NavierStokesTwoBubbles(NavierStokesBubbles):
    """
    Here we simulate a homogeneous domain containing two bubbles, one of higher
    temperature beneath the other of colder temperature.
    Their repsective temperatures will cause the two bubbles to move towards one
    another and collide, causing mixing effects.
    """

    def evaluate_perturbation(self):
        return """
  // evaluate perturbation
  tarch::la::Vector<DIMENSIONS, double> hotBubbleCenter = {
    500, 300
#if DIMENSIONS == 3
    ,300
#endif
  };

  tarch::la::Vector<DIMENSIONS, double> coldBubbleCenter = {
    560, 640
#if DIMENSIONS == 3
    ,640
#endif
  };

  constexpr double decay = 50.;
  constexpr double tempDifferenceHot  = 0.5;
  constexpr double tempDifferenceCold = -0.15;

  const auto d1 = tarch::la::norm2(x-hotBubbleCenter) - 150; // Hot bubble has a radius of 150
  auto perturbation = d1 <= 0 ? tempDifferenceHot
    : tempDifferenceHot * std::exp(-(d1 * d1) / (decay * decay));

  const auto d2 = tarch::la::norm2(x-coldBubbleCenter) - 0; // Cold bubble has a radius of 0
  perturbation += tempDifferenceCold * std::exp(-(d2 * d2) / (decay * decay));
"""
