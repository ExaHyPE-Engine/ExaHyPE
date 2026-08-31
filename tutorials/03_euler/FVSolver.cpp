// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVSolver.h"

namespace {
  /**
   * Returns the half-thickness of a symmetric airfoil at the distance x from its
   * leading edge.
   */
  double airfoilSymmetric(const double x, const double chordLength = 100, const double relativeThickness = 0.12) {
    const double relativePosition = x / chordLength;
    return 5.0 * relativeThickness * chordLength
      * (0.2969 * std::sqrt(relativePosition)
         + ((((-0.1015) * relativePosition + 0.2843) * relativePosition - 0.3516) * relativePosition - 0.1260)
           * relativePosition);
  }
} // namespace

void tutorials::exahype2::euler::FVSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  const bool   inChord = cellCentre[0] >= 0.0 and cellCentre[0] <= 100.0;
  const double y       = inChord ? airfoilSymmetric(cellCentre[0]) : 0.0;

  Q[Shortcuts::rho]   = (inChord and y > cellCentre[1] and -y < cellCentre[1]) ? 1000.0 : 1.0;
  Q[Shortcuts::j + 0] = 1.0;
  Q[Shortcuts::j + 1] = 0.0;
  Q[Shortcuts::E]     = 2.5;
}

::exahype2::RefinementCommand tutorials::exahype2::euler::FVSolver::refinementCriterion(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time
) {
  // Q holds no valid state during grid construction, so only cellCentre enters the decision.
  constexpr double RefinementMargin = 3.0;
  constexpr double LeadingEdge      = 0.0;
  constexpr double TrailingEdge     = 100.0;
  constexpr double WakeLength       = 15.0;

  const bool nearChord = cellCentre[0] >= LeadingEdge - RefinementMargin and cellCentre[0] <= TrailingEdge + WakeLength;
  const double halfThickness =
    (cellCentre[0] >= LeadingEdge and cellCentre[0] <= TrailingEdge) ? airfoilSymmetric(cellCentre[0]) : 0.0;

  if (nearChord and std::abs(cellCentre[1]) <= halfThickness + RefinementMargin) {
    return ::exahype2::RefinementCommand::Refine;
  }
  return ::exahype2::RefinementCommand::Keep;
}

void tutorials::exahype2::euler::FVSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  if (normal == 0 and faceCentre[0] == DomainOffset[0]) {
    Qoutside[Shortcuts::rho]   = 1.0;
    Qoutside[Shortcuts::j + 0] = 1.0;
    Qoutside[Shortcuts::j + 1] = 0.0;
    Qoutside[Shortcuts::E]     = 2.5;
  } else {
    Qoutside[Shortcuts::rho]   = Qinside[Shortcuts::rho];
    Qoutside[Shortcuts::j + 0] = Qinside[Shortcuts::j + 0];
    Qoutside[Shortcuts::j + 1] = Qinside[Shortcuts::j + 1];
    Qoutside[Shortcuts::E]     = Qinside[Shortcuts::E];
  }
}

double tutorials::exahype2::euler::FVSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  const double     irho  = 1.0 / Q[Shortcuts::rho];
  constexpr double Gamma = 1.4;
  const double     p     = (Gamma - 1)
    * (Q[Shortcuts::E]
       - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  const double c = std::sqrt(Gamma * p * irho);
  const double u = Q[Shortcuts::j + normal] * irho;

  return std::max(std::abs(u - c), std::abs(u + c));
}

void tutorials::exahype2::euler::FVSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  const double     irho  = 1.0 / Q[Shortcuts::rho];
  constexpr double Gamma = 1.4;
  const double     p     = (Gamma - 1)
    * (Q[Shortcuts::E]
       - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  F[Shortcuts::rho]   = Q[Shortcuts::j + normal];
  F[Shortcuts::j + 0] = Q[Shortcuts::j + normal] * Q[Shortcuts::j + 0] * irho;
  F[Shortcuts::j + 1] = Q[Shortcuts::j + normal] * Q[Shortcuts::j + 1] * irho;
  F[Shortcuts::E]     = Q[Shortcuts::j + normal] * irho * (Q[Shortcuts::E] + p);

  F[Shortcuts::j + normal] += p;
}
