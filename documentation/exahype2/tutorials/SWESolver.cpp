// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWESolver.h"

tarch::logging::Log tutorials::exahype2::swe::SWESolver::_log("tutorials::exahype2::swe::SWESolver");

void tutorials::exahype2::swe::SWESolver::initialCondition(
  [[maybe_unused]] double* const NOALIAS                        Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const bool                                   gridIsConstructed
) {
  Q[Shortcuts::hu + 0] = 0.0; // v_x
  Q[Shortcuts::hu + 1] = 0.0; // v_y
  Q[Shortcuts::h]      = 1.0;
  Q[Shortcuts::b]      = (tarch::la::norm2(x) < 0.5 ? 0.1 : 0.0);
}

void tutorials::exahype2::swe::SWESolver::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS                  Qinside,  // Qinside[4+0]
  [[maybe_unused]] double* const NOALIAS                        Qoutside, // Qoutside[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  Qoutside[Shortcuts::h]      = 1.0; // h
  Qoutside[Shortcuts::hu + 0] = 0.0; // v_x
  Qoutside[Shortcuts::hu + 1] = 0.0; // v_y
  Qoutside[Shortcuts::b]      = 0.0; // b
}

::exahype2::RefinementCommand tutorials::exahype2::swe::SWESolver::refinementCriterion(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t
) {
  return tarch::la::norm2(x) < 0.6 ? ::exahype2::RefinementCommand::Refine : ::exahype2::RefinementCommand::Keep;
}

double tutorials::exahype2::swe::SWESolver::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  constexpr double g = 9.81;
  const double     u = Q[Shortcuts::hu + normal] / Q[Shortcuts::h];
  const double     c = std::sqrt(g * Q[Shortcuts::h]);
  return std::max(std::abs(u + c), std::abs(u - c));
}

void tutorials::exahype2::swe::SWESolver::flux(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        F // F[4]
) {
  double ih    = 1.0 / Q[Shortcuts::h];
  F[Shortcuts::h]      = Q[Shortcuts::hu + normal];
  F[Shortcuts::hu + 0] = Q[Shortcuts::hu + normal] * Q[Shortcuts::hu + 0] * ih;
  F[Shortcuts::hu + 1] = Q[Shortcuts::hu + normal] * Q[Shortcuts::hu + 1] * ih;
  F[Shortcuts::b]      = 0.0;
}

void tutorials::exahype2::swe::SWESolver::nonconservativeProduct(
  [[maybe_unused]] const double* const NOALIAS                  Q,      // Q[4+0]
  [[maybe_unused]] const double* const NOALIAS                  deltaQ, // deltaQ[4+0]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        BTimesDeltaQ // BTimesDeltaQ[4]
) {
  constexpr double g = 9.81;
  BTimesDeltaQ[Shortcuts::h] = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[Shortcuts::hu + 0] = g * Q[Shortcuts::h] * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
    BTimesDeltaQ[Shortcuts::hu + 1] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[Shortcuts::hu + 0] = 0.0;
    BTimesDeltaQ[Shortcuts::hu + 1] = g * Q[Shortcuts::h] * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
    break;
  }
  BTimesDeltaQ[Shortcuts::b] = 0.0;
}
