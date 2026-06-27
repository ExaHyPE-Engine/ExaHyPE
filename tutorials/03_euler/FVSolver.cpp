// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Worked Finite Volumes implementation for the compressible Euler equations.
// The conserved variables are addressed through the generated Shortcuts enum:
//   Shortcuts::rho      density        (1 component)
//   Shortcuts::j        momentum       (2 components: +0 is x, +1 is y)
//   Shortcuts::E        total energy   (1 component)
#include "FVSolver.h"

#include <algorithm>
#include <cmath>

// Half-thickness of a symmetric NACA airfoil. c: chord length, t: maximum
// relative thickness.
double airfoilSymmetric(double x, double c = 100, double t = 0.12) {
  return 5.0 * t * c
         * (0.2969 * std::sqrt(x / c) + ((((-0.1015) * (x / c) + 0.2843) * (x / c) - 0.3516) * (x / c) - 0.1260) * (x / c));
}

void tutorials::exahype2::euler::FVSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  const bool   inChord = cellCentre[0] >= 0.0 && cellCentre[0] <= 100.0;
  const double y       = inChord ? airfoilSymmetric(cellCentre[0]) : 0.0;

  // Dense gas blob inside the airfoil contour, uniform free stream outside.
  Q[Shortcuts::rho]   = (inChord && y > cellCentre[1] && -y < cellCentre[1]) ? 1000.0 : 1.0;
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
  // Static, purely geometric refinement: refine the patches that hug the airfoil
  // (plus a short trailing wake) so the surface and the flow around it are well
  // resolved, while the far field stays coarse. The mesh therefore stays fixed
  // for the whole run. We deliberately ignore Q -- it is not even valid during
  // grid construction -- and use only the patch position and the NACA contour.
  constexpr double RefinementMargin = 3.0;    // band thickness around the contour
  constexpr double LeadingEdge      = 0.0;
  constexpr double TrailingEdge     = 100.0;
  constexpr double WakeLength       = 15.0;   // keep refining a little downstream

  const bool nearChord = cellCentre[0] >= LeadingEdge - RefinementMargin
                      && cellCentre[0] <= TrailingEdge + WakeLength;
  const double halfThickness = (cellCentre[0] >= LeadingEdge && cellCentre[0] <= TrailingEdge)
                                 ? airfoilSymmetric(cellCentre[0])
                                 : 0.0;

  if (nearChord && std::abs(cellCentre[1]) <= halfThickness + RefinementMargin) {
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
  if (normal == 0 && faceCentre[0] == DomainOffset[0]) {
    // Fixed inflow state on the left face.
    Qoutside[Shortcuts::rho]   = 1.0;
    Qoutside[Shortcuts::j + 0] = 1.0;
    Qoutside[Shortcuts::j + 1] = 0.0;
    Qoutside[Shortcuts::E]     = 2.5;
  } else {
    // Outflow: copy the inside state.
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
  constexpr double gamma = 1.4;
  const double     p     = (gamma - 1)
    * (Q[Shortcuts::E] - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  const double c = std::sqrt(gamma * p * irho);
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
  constexpr double gamma = 1.4;
  const double     p     = (gamma - 1)
    * (Q[Shortcuts::E] - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  F[Shortcuts::rho]   = Q[Shortcuts::j + normal];
  F[Shortcuts::j + 0] = Q[Shortcuts::j + normal] * Q[Shortcuts::j + 0] * irho;
  F[Shortcuts::j + 1] = Q[Shortcuts::j + normal] * Q[Shortcuts::j + 1] * irho;
  F[Shortcuts::E]     = Q[Shortcuts::j + normal] * irho * (Q[Shortcuts::E] + p);

  F[Shortcuts::j + normal] += p;
}
