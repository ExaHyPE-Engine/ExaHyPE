// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

::tarch::logging::Log applications::exahype2::swe::SWE::_log(
  "applications::exahype2::swe::SWE"
);

using s = applications::exahype2::swe::VariableShortcuts;

void applications::exahype2::swe::SWE::initialCondition(
  double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  bool                                           gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  static constexpr double h0    = 0.32;
  static constexpr double A     = 0.064;
  static constexpr double xc    = 12.5;
  static constexpr double yc    = 15.0;
  static constexpr double rc    = 3.6;
  static constexpr double gamma = std::sqrt((3.0 * A) / (4.0 * h0));

  auto r = [](double x, double y) {
    return std::sqrt(std::pow(x - xc, 2) + std::pow(y - yc, 2));
  };

  Q[s::h] = h0 + (A / h0 * std::pow(1.0 / (std::cosh(gamma * (x(0) - 2.5))), 2));
  Q[s::b] = r(x(0), x(1)) <= rc ? 0.93 * (1.0 - r(x(0), x(1)) / rc) : 0.0;
}


void applications::exahype2::swe::SWE::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  int                                            normal
) {
  Qoutside[s::h]      = Qinside[s::h];
  Qoutside[s::hu + 0] = -Qinside[s::hu + 0];
  Qoutside[s::hu + 1] = -Qinside[s::hu + 1];
  Qoutside[s::b]      = Qinside[s::b];
}


::exahype2::RefinementCommand applications::exahype2::swe::SWE::
  refinementCriterion(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t
  ) {
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[0]);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[1]);

  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[0]);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[1]);

  static constexpr double xc = 12.5;
  static constexpr double yc = 15.0;
  static constexpr double rc = 3.6;

  auto r = [](double x, double y) {
    return std::sqrt(std::pow(x - xc, 2) + std::pow(y - yc, 2));
  };

  return r(x(0), x(1)) <= rc
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
}
