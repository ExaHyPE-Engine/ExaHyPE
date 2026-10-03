// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  struct Limits;
} // namespace exahype2

/**
 * Time and mesh bounds of one solver. A fixed time step size scales with the smallest mesh width
 * divided by maxAdmissibleMeshWidth.
 */
struct exahype2::Limits final {
  double terminalTime{std::numeric_limits<double>::max()};
  double minAdmissiblePatchSize{0.0};
  double maxAdmissiblePatchSize{std::numeric_limits<double>::max()};
  double maxAdmissibleMeshWidth{std::numeric_limits<double>::max()};
};
