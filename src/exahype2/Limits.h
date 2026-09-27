// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  struct Limits;
} // namespace exahype2

/**
 * The bounds a run sets on one solver: The time stamp at which the run ends, the patch sizes
 * between which the refinement votes keep the mesh, and the largest admissible mesh width,
 * which scales the time step size of a fixed rule.
 */
struct exahype2::Limits final {
  double terminalTime{std::numeric_limits<double>::max()};
  double minAdmissiblePatchSize{0.0};
  double maxAdmissiblePatchSize{std::numeric_limits<double>::max()};
  double maxAdmissibleMeshWidth{std::numeric_limits<double>::max()};
};
