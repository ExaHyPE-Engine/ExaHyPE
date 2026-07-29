// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonState.h"

#include "peano4/grid/PeanoCurve.h"

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

  out << ", Inverted=" << (getInverted() ? "true" : "false");

  out << ", EvenFlags=[";
  for (int d = 0; d < DIMENSIONS; d++) {
    if (d > 0) {
      out << ",";
    }
    out << (getEvenFlag(d) ? "1" : "0");
  }
  out << "]";

  out << ", AccessNumber=[";
  for (std::size_t i = 0; i < accessNumber.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << static_cast<int>(accessNumber[i]);
  }
  out << "]";

  out << "}";

  return out.str();
}

/**
 * Takes a state describing a node in the tree and derives the @f$ 3^d @f$
 * states on the next finer level. The routine is the grammar generation of
 * Bader et al., and it recurses along the spatial dimensions.
 *
 * A child's slot in fineGridStates is its position within the parent, with the
 * first axis running fastest, so a caller can index the result by position.
 * The recursion visits the children along the Peano space-filling curve, which
 * is what gives each of them its entry and exit face.
 */
void peano4::grid::AutomatonState::refineState(
  const AutomatonState&              coarseGrid,
  AutomatonState                     fineGridStates[THREE_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> fineGridStatesPosition,
  int                                axis
) {
  if (axis == -1) {
    int arrayIndex                   = peano4::dLinearised(fineGridStatesPosition, 3);
    fineGridStates[arrayIndex]       = coarseGrid;
    fineGridStates[arrayIndex].level = coarseGrid.level + 1;
  } else {
    assertion(axis >= 0);
    assertion(axis < DIMENSIONS);

    AutomatonState firstCell  = coarseGrid;
    AutomatonState secondCell = coarseGrid;
    AutomatonState thirdCell  = coarseGrid;

    if (PeanoCurve::isTraversePositiveAlongAxis(coarseGrid, axis)) {
      PeanoCurve::setExitFace(firstCell, axis);
    } else {
      PeanoCurve::setEntryFace(firstCell, axis);
    }
    firstCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 0;
    firstCell.x[axis]            = coarseGrid.x[axis];
    refineState(firstCell, fineGridStates, fineGridStatesPosition, axis - 1);

    PeanoCurve::invertEvenFlag(secondCell, axis);
    PeanoCurve::setEntryFace(secondCell, axis);
    PeanoCurve::setExitFace(secondCell, axis);
    secondCell.h[axis]           = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 1;
    secondCell.x[axis]           = coarseGrid.x[axis] + coarseGrid.h[axis] / 3.0;
    refineState(secondCell, fineGridStates, fineGridStatesPosition, axis - 1);

    if (PeanoCurve::isTraversePositiveAlongAxis(coarseGrid, axis)) {
      PeanoCurve::setEntryFace(thirdCell, axis);
    } else {
      PeanoCurve::setExitFace(thirdCell, axis);
    }
    thirdCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 2;
    thirdCell.x[axis]            = coarseGrid.x[axis] + 2.0 * coarseGrid.h[axis] / 3.0;
    refineState(thirdCell, fineGridStates, fineGridStatesPosition, axis - 1);
  }

  if (axis == DIMENSIONS - 1) {
    for (int i = 0; i < THREE_POWER_D; i++) {
      for (int d = 0; d < TWO_TIMES_D; d++) {
        assertion(
          fineGridStates[i].accessNumber[d] > -2 * DIMENSIONS,
          i,
          d,
          fineGridStates[i].toString(),
          coarseGrid.toString(),
          THREE_POWER_D,
          TWO_TIMES_D
        );
        assertion(
          fineGridStates[i].accessNumber[d] < 2 * DIMENSIONS,
          i,
          d,
          fineGridStates[i].toString(),
          coarseGrid.toString(),
          THREE_POWER_D,
          TWO_TIMES_D
        );
      }
    }
  }
}
