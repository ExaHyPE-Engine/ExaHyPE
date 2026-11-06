// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonState.h"

#include "tarch/Assertions.h"

static_assert(std::is_standard_layout_v<peano4::grid::AutomatonState>);
static_assert(std::is_trivially_copyable_v<peano4::grid::AutomatonState>);

std::ostream&
operator<<(std::ostream& out, const peano4::grid::AutomatonState& state) {
  return (out << state.toString());
}

std::string
peano4::grid::AutomatonState::toString() const {
  std::ostringstream out;
  out << "AutomatonState{";

  out << "Level=" << level;

  out << ", x=[";
  for (auto i = 0; i < x.getSize(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << x[i];
  }
  out << "]";

  out << ", h=[";
  for (auto i = 0; i < h.getSize(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << h[i];
  }
  out << "]";

  out << ", CurveState=" << static_cast<int>(curveState);

  out << "}";

  return out.str();
}

/**
 * Derives the states of the children of a cell. The array fineGridStates stores the children by
 * their linearised position in the parent, with the first axis running fastest.
 */
void
peano4::grid::AutomatonState::refineState(
  const peano4::grid::AutomatonState& coarseGrid,
  peano4::grid::AutomatonState        fineGridStates[peano4::sfc::ActiveCurve::ChildrenPerCell]
) {
  ASSERTION(coarseGrid.level >= 0, coarseGrid.toString());
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  for (const auto& child : peano4::sfc::ActiveCurve::getChildren(coarseGrid.curveState)) {
    ASSERTION(
      child.linearisedPosition < peano4::sfc::ActiveCurve::ChildrenPerCell,
      static_cast<int>(child.linearisedPosition),
      coarseGrid.toString()
    );
    auto& fineGridState      = fineGridStates[child.linearisedPosition];
    fineGridState.level      = coarseGrid.level + 1;
    fineGridState.curveState = child.state;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      ASSERTION(coarseGrid.h(axis) > 0.0, coarseGrid.toString(), axis);
      fineGridState.h(axis) = coarseGrid.h(axis) / SubdivisionFactor;
      fineGridState.x(axis) = coarseGrid.x(axis) + child.position[axis] * coarseGrid.h(axis) / SubdivisionFactor;
    }
  }
}
