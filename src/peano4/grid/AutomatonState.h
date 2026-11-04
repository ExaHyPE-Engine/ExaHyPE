// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct AutomatonState;
} // namespace peano4::grid

/**
 * Describes one cell of the traversal: Its geometry, its level and the state
 * of the space-filling curve within it. The curve state selects the order in
 * which refineState() visits the children. The members run from the widest to
 * the narrowest and leave the least padding.
 */
struct peano4::grid::AutomatonState {
  tarch::la::Vector<DIMENSIONS, double> x{};
  tarch::la::Vector<DIMENSIONS, double> h{};

  int level{0};

  std::uint8_t curveState{peano4::sfc::ActiveCurve::RootState};

  AutomatonState() noexcept = default;

  [[nodiscard]] std::string toString() const;

  static void refineState(
    const AutomatonState& coarseGrid,
    AutomatonState        fineGridStates[peano4::sfc::ActiveCurve::ChildrenPerCell]
  );
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::AutomatonState& state);
