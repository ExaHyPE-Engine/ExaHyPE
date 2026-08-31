// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InitialCondition.h"

#include "VariableShortcutsCCZ4Solver.h"

namespace {
  using Shortcuts = applications::exahype2::exagrype::VariableShortcutsCCZ4Solver;
} // namespace

void applications::exahype2::exagrype::initialCondition(
  const tarch::la::Vector<3, double>& x,
  double                              t,
  int                                 nVars,
  double* NOALIAS                     Q
) {
  constexpr double Pi         = tarch::la::PI;
  constexpr double PeakNumber = 2.0;
  constexpr double Amplitude  = 0.1;

  const double phase  = PeakNumber * Pi * (x[0] - t);
  const double hh     = 1.0 - Amplitude * std::sin(phase);
  const double dxH    = -PeakNumber * Pi * Amplitude * std::cos(phase);
  const double phi    = std::pow(1.0 / hh, 1.0 / 6.0);
  const double dxphi  = -std::pow(hh, -7.0 / 6.0) * dxH / 6.0;
  const double kxx    = -0.5 * PeakNumber * Pi * Amplitude * std::cos(phase) / std::sqrt(hh);
  const double traceK = kxx / hh;

  std::fill_n(Q, nVars, 0.0);

  Q[Shortcuts::gamma_tilde + 0]      = phi * phi * hh;
  Q[Shortcuts::gamma_tilde + 3]      = phi * phi;
  Q[Shortcuts::gamma_tilde + 5]      = phi * phi;
  Q[Shortcuts::A_tilde + 0]          = phi * phi * (kxx - 1.0 / 3.0 * traceK * hh);
  Q[Shortcuts::A_tilde + 3]          = phi * phi * (-1.0 / 3.0 * traceK);
  Q[Shortcuts::A_tilde + 5]          = phi * phi * (-1.0 / 3.0 * traceK);
  Q[Shortcuts::gamma_hat + 0]        = 2.0 / (3.0 * std::pow(hh, 5.0 / 3.0)) * dxH;
  Q[Shortcuts::alpha]                = std::sqrt(hh);
  Q[Shortcuts::grad_alpha + 0]       = 0.5 * dxH * std::pow(hh, -0.5);
  Q[Shortcuts::grad_gamma_tilde + 0] = std::pow(hh, -1.0 / 3.0) * dxH / 3.0;
  Q[Shortcuts::grad_gamma_tilde + 3] = phi * dxphi;
  Q[Shortcuts::grad_gamma_tilde + 5] = phi * dxphi;
  Q[Shortcuts::K]                    = traceK;
  Q[Shortcuts::phi]                  = phi;
  Q[Shortcuts::grad_phi + 0]         = dxphi;
}
