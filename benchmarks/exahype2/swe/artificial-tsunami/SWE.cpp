// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

tarch::logging::Log applications::exahype2::swe::SWE::_log(
  "applications::exahype2::swe::SWE"
);

using s = applications::exahype2::swe::VariableShortcuts;

void applications::exahype2::swe::SWE::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  bool                                         gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  Q[s::h] = INITIAL_WATER_HEIGHT;

  auto displacementX = [](double x) {
    return std::sin((x / 500.0 + 1.0) * std::numbers::pi);
  };

  auto displacementY = [](double y) { return -std::pow(y / 500.0, 2) + 1.0; };

  auto displacement = [&](double x, double y) {
    return 5.0 * displacementX(x) * displacementY(y);
  };

  const double bathymetryAfterEarthquake = INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE
                                           + displacement(x(0), x(1));

  if (std::abs(x(0)) <= 500 and std::abs(x(1)) <= 500) { // Center of domain is
                                                         // [0, 0]
    Q[s::b] = bathymetryAfterEarthquake;
  } else {
    Q[s::b] = INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE;
  }
}


void applications::exahype2::swe::SWE::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::swe::SWE::
  refinementCriterion(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t
  ) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (std::abs(x(0)) <= 500 and std::abs(x(1)) <= 500) { // Center of domain is
                                                         // [0, 0]
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}
