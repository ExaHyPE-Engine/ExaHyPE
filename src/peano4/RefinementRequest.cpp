// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementRequest.h"

static_assert(std::is_standard_layout_v<peano4::RefinementRequest>);
static_assert(std::is_trivially_copyable_v<peano4::RefinementRequest>);

static_assert(std::is_trivially_copy_constructible_v<peano4::RefinementRequest>);
static_assert(std::is_trivially_copy_assignable_v<peano4::RefinementRequest>);
static_assert(std::is_trivially_move_constructible_v<peano4::RefinementRequest>);
static_assert(std::is_trivially_move_assignable_v<peano4::RefinementRequest>);
static_assert(std::is_trivially_destructible_v<peano4::RefinementRequest>);

static_assert(std::is_nothrow_move_constructible_v<peano4::RefinementRequest>);
static_assert(std::is_nothrow_move_assignable_v<peano4::RefinementRequest>);

static_assert(offsetof(peano4::RefinementRequest, refinementControl) == 0);
static_assert(offsetof(peano4::RefinementRequest, level) > offsetof(peano4::RefinementRequest, refinementControl));
static_assert(offsetof(peano4::RefinementRequest, latticeIndex) > offsetof(peano4::RefinementRequest, level));

/**
 * Orders requests by refinement control, then level, then lattice position. The key covers every
 * field, so sorting yields the same sequence for every insertion order.
 */
bool
peano4::RefinementRequest::operator<(const RefinementRequest& other) const noexcept {
  if (refinementControl != other.refinementControl) {
    return refinementControl < other.refinementControl;
  }
  if (level != other.level) {
    return level < other.level;
  }
  for (int axis = 0; axis < DIMENSIONS; ++axis) {
    if (latticeIndex[axis] != other.latticeIndex[axis]) {
      return latticeIndex[axis] < other.latticeIndex[axis];
    }
  }
  return false;
}

bool
peano4::RefinementRequest::operator==(const RefinementRequest& other) const noexcept {
  return refinementControl == other.refinementControl and level == other.level and latticeIndex == other.latticeIndex;
}

std::ostream&
operator<<(std::ostream& out, const peano4::RefinementRequest& request) {
  return (out << request.toString());
}

std::string
peano4::RefinementRequest::toString() const {
  std::ostringstream out;
  out << "RefinementRequest{";
  out << "RefinementControl=" << (getRefinementControl() == RefinementControl::Refine ? "Refine" : "Erase");
  out << ", Level=" << level;
  out << ", LatticeIndex=[";
  for (int axis = 0; axis < DIMENSIONS; ++axis) {
    if (axis > 0) {
      out << ',';
    }
    out << latticeIndex[axis];
  }
  out << "]}";
  return out.str();
}
