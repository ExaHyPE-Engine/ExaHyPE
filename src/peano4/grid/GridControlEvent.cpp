// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEvent.h"

#include "tarch/Assertions.h"

static_assert(std::is_standard_layout_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridControlEvent>);

static_assert(std::is_trivially_copy_constructible_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_copy_assignable_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_move_constructible_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_move_assignable_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_destructible_v<peano4::grid::GridControlEvent>);

static_assert(std::is_nothrow_move_constructible_v<peano4::grid::GridControlEvent>);
static_assert(std::is_nothrow_move_assignable_v<peano4::grid::GridControlEvent>);

static_assert(offsetof(peano4::grid::GridControlEvent, refinementControl) == 0);
static_assert(
  offsetof(peano4::grid::GridControlEvent, level) > offsetof(peano4::grid::GridControlEvent, refinementControl)
);
static_assert(offsetof(peano4::grid::GridControlEvent, latticeIndex) > offsetof(peano4::grid::GridControlEvent, level));

/**
 * Names the cell a request was raised for.
 *
 * Cells sit on a triadic lattice anchored at the domain offset, so the level is how often
 * the cell count per axis divides by three, and rounding the cell's lower corner over the
 * cell width recovers its lattice position.
 */
peano4::grid::GridControlEvent peano4::grid::GridControlEvent::forCell(
  const RefinementControl                      control,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellH,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainWidth
) {
  peano4::grid::GridControlEvent request;
  request.setRefinementControl(control);

  auto cellsPerAxis = std::llround(domainWidth(0) / cellH(0));
  assertion(cellsPerAxis >= 1, cellCentre, cellH, domainWidth);

  std::int32_t level = 0;
  while (cellsPerAxis > 1) {
    assertion(cellsPerAxis % 3 == 0, cellCentre, cellH, domainWidth);
    cellsPerAxis /= 3;
    level++;
  }
  request.level = level;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto position        = (cellCentre(axis) - 0.5 * cellH(axis) - domainOffset(axis)) / cellH(axis);
    request.latticeIndex(axis) = static_cast<int>(std::llround(position));
  }
  return request;
}

/**
 * Orders requests by refinement control, level, then lattice position.
 *
 * The key covers every field of the request, so the sorted sequence is independent
 * of insertion order. A caller uses it to form a canonical request set.
 */
bool peano4::grid::GridControlEvent::operator<(const GridControlEvent& other) const noexcept {
  if (refinementControl != other.refinementControl) {
    return refinementControl < other.refinementControl;
  }
  if (level != other.level) {
    return level < other.level;
  }
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (latticeIndex[d] != other.latticeIndex[d]) {
      return latticeIndex[d] < other.latticeIndex[d];
    }
  }
  return false;
}

/**
 * Compares the requested command and the cell it names.
 *
 * A caller compares a freshly assembled request set against the set the mesh already
 * realises, so it can drop a resubmission that cannot change the grid.
 */
bool peano4::grid::GridControlEvent::operator==(const GridControlEvent& other) const noexcept {
  return refinementControl == other.refinementControl and level == other.level and latticeIndex == other.latticeIndex;
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridControlEvent& event) {
  return (out << event.toString());
}

std::string peano4::grid::GridControlEvent::toString() const {
  auto refinementControlToString = [](const RefinementControl control) {
    return (control == RefinementControl::Refine) ? "Refine" : "Erase";
  };

  std::ostringstream out;
  out << "GridControlEvent{";
  out << "RefinementControl=" << refinementControlToString(getRefinementControl());
  out << ", Level=" << level;

  out << ", LatticeIndex=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << latticeIndex[d];
  }
  out << "]}";
  return out.str();
}
