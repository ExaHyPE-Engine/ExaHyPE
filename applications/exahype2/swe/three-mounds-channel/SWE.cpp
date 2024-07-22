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

  const double m1 = 1.0
                    - 0.10
                        * std::sqrt(
                          (x(0) - 30.0) * (x(0) - 30.0)
                          + (x(1) - 22.5) * (x(1) - 22.5)
                        );
  const double m2 = 1.0
                    - 0.10
                        * std::sqrt(
                          (x(0) - 30.0) * (x(0) - 30.0)
                          + (x(1) - 7.50) * (x(1) - 7.50)
                        );
  const double m3 = 1.0
                    - 0.28
                        * std::sqrt(
                          (x(0) - 47.5) * (x(0) - 47.5)
                          + (x(1) - 15.0) * (x(1) - 15.0)
                        );

  Q[s::h] = x[0] <= 15.0 ? INITIAL_WATER_HEIGHT_DAM : 0.0;
  Q[s::b] = std::max({0.0, m1, m2, m3});
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

  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) >= 20 && x(0) <= 50) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}
