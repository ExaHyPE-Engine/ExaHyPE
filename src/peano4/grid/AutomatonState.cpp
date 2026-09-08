// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonState.h"

#include "peano4/Loop.h"
#include "peano4/grid/PeanoCurve.h"
#include "tarch/Assertions.h"

static_assert(std::is_standard_layout_v<peano4::grid::AutomatonState>);
static_assert(std::is_trivially_copyable_v<peano4::grid::AutomatonState>);

std::ostream& operator<<(std::ostream& out, const peano4::grid::AutomatonState& state) {
  return (out << state.toString());
}

std::string peano4::grid::AutomatonState::toString() const {
  std::ostringstream out;
  out << "AutomatonState{";

  out << "Level=" << level;

  out << ", x=[";
  for (auto i = 0; i < x.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << x[i];
  }
  out << "]";

  out << ", h=[";
  for (auto i = 0; i < h.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << h[i];
  }
  out << "]";

  out << ", EvenFlags=[";
  for (int d = 0; d < DIMENSIONS; d++) {
    if (d > 0) {
      out << ",";
    }
    out << (getEvenFlag(d) ? "1" : "0");
  }
  out << "]";

  out << "}";

  return out.str();
}

/**
 * Derives the @f$ 3^d @f$ states on the next finer level from one state. A
 * child's slot in fineGridStates is its position within the parent, with the
 * first axis running fastest. The recursion visits the children along the
 * Peano space-filling curve, which gives each of them its entry and exit face.
 */
void peano4::grid::AutomatonState::refineState(
  const peano4::grid::AutomatonState& coarseGrid,
  peano4::grid::AutomatonState        fineGridStates[THREE_POWER_D],
  tarch::la::Vector<DIMENSIONS, int>  fineGridStatesPosition,
  int                                 axis
) {
  if (axis == -1) {
    int arrayIndex                   = peano4::dLinearised(fineGridStatesPosition, 3);
    fineGridStates[arrayIndex]       = coarseGrid;
    fineGridStates[arrayIndex].level = coarseGrid.level + 1;
  } else {
    assertion(axis >= 0);
    assertion(axis < DIMENSIONS);

    peano4::grid::AutomatonState firstCell  = coarseGrid;
    peano4::grid::AutomatonState secondCell = coarseGrid;
    peano4::grid::AutomatonState thirdCell  = coarseGrid;

    firstCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 0;
    firstCell.x[axis]            = coarseGrid.x[axis];
    refineState(firstCell, fineGridStates, fineGridStatesPosition, axis - 1);

    secondCell.flipEvenFlag(axis);
    secondCell.h[axis]           = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 1;
    secondCell.x[axis]           = coarseGrid.x[axis] + coarseGrid.h[axis] / 3.0;
    refineState(secondCell, fineGridStates, fineGridStatesPosition, axis - 1);

    thirdCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 2;
    thirdCell.x[axis]            = coarseGrid.x[axis] + 2.0 * coarseGrid.h[axis] / 3.0;
    refineState(thirdCell, fineGridStates, fineGridStatesPosition, axis - 1);
  }
}
