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
  offsetof(peano4::grid::GridControlEvent, offset) > offsetof(peano4::grid::GridControlEvent, refinementControl)
);
static_assert(offsetof(peano4::grid::GridControlEvent, width) > offsetof(peano4::grid::GridControlEvent, offset));
static_assert(offsetof(peano4::grid::GridControlEvent, targetH) > offsetof(peano4::grid::GridControlEvent, width));

namespace {
  /**
   * Try to fuse two events into a single box.
   *
   * The union of two axis-aligned boxes is itself a box exactly when they coincide in every
   * axis but one and overlap or touch in the remaining axis. Only then do we replace the pair
   * by their bounding box, which then equals their union, so the fused box covers no cell outside
   * the two originals. Boxes whose union is not a single box (different targetH, diagonal
   * neighbours, a gap between them) are left untouched.
   *
   * All comparisons are exact: the events of one refinement level derive from the same
   * deterministic subdivision arithmetic on every tree and rank, so coinciding boxes match
   * bitwise, and a tolerance would only make the merge outcome data-dependent. The fusion is
   * also operand-symmetric - the operands agree bitwise on every axis but the merge axis,
   * where min/max are symmetric - so the input order cannot leak into the result.
   *
   * @return true, with the fused box in @p merged, if the pair forms a single box.
   */
  bool tryMergeIntoSingleBox(
    const peano4::grid::GridControlEvent& a,
    const peano4::grid::GridControlEvent& b,
    peano4::grid::GridControlEvent&       merged
  ) {
    // Events on the same level share an exact targetH; a different targetH must not be fused.
    if (a.targetH != b.targetH) {
      return false;
    }

    int mergeAxis = -1;
    for (int d = 0; d < DIMENSIONS; ++d) {
      const bool sameInterval = (a.offset[d] == b.offset[d]) and (a.width[d] == b.width[d]);
      if (sameInterval) {
        continue;
      }
      if (mergeAxis != -1) {
        return false; // Boxes differ in more than one axis => union is not a single box
      }
      mergeAxis = d;
    }

    if (mergeAxis == -1) {
      merged = a; // Boxes are identical => deduplicate
      return true;
    }

    const double aLo = a.offset[mergeAxis];
    const double aHi = a.offset[mergeAxis] + a.width[mergeAxis];
    const double bLo = b.offset[mergeAxis];
    const double bHi = b.offset[mergeAxis] + b.width[mergeAxis];
    if (aHi < bLo or bHi < aLo) {
      return false; // A genuine gap between them => merging would cover extra cells
    }

    merged                   = a;
    merged.offset[mergeAxis] = std::min(aLo, bLo);
    merged.width[mergeAxis]  = std::max(aHi, bHi) - merged.offset[mergeAxis];
    return true;
  }

  /**
   * Greedily coalesce same-type events.
   *
   * We sort once by (refinement control, targetH, offset, width), which places a level's boxes
   * contiguously and lines up face-touching neighbours, then repeatedly sweep the vector, fusing
   * each event into the most recently kept box whenever tryMergeIntoSingleBox() succeeds. A solid
   * region collapses axis by axis - runs along the last axis first, then the resulting columns,
   * then the slabs - over about DIMENSIONS sweeps. Fusion keeps the (already smallest) offset of
   * the box it extends, so the vector stays sorted and no further sort is needed; we stop once a
   * sweep fuses nothing. Every sweep preserves the covered region.
   */
  void mergeAdjacentRefinementEvents(std::vector<peano4::grid::GridControlEvent>& events) {
    std::sort(events.begin(), events.end());

    std::vector<peano4::grid::GridControlEvent> merged;
    merged.reserve(events.size());
    bool changed = true;
    while (changed) {
      changed = false;
      merged.clear();
      for (const auto& event : events) {
        peano4::grid::GridControlEvent combined;
        if (not merged.empty() and tryMergeIntoSingleBox(merged.back(), event, combined)) {
          merged.back() = combined;
          changed       = true;
        } else {
          merged.push_back(event);
        }
      }
      events.swap(merged);
    }
  }
} // namespace

/**
 * Exact lexicographic ordering by refinement control, targetH, offset, then width.
 *
 * This is the order merge() sorts with: events of one refinement level (identical targetH) end
 * up contiguous, and within a level face-touching boxes become neighbours, so the greedy sweep
 * can fuse them. The comparisons are exact, and the key covers every field, so geometrically
 * distinct events never tie: the sorted sequence is independent of the order in which trees
 * and ranks contributed the events. A tolerance-based or incomplete key would let distinct
 * events tie and leak the input order into the merge, breaking mirror symmetry under
 * OpenMP/MPI decompositions.
 */
bool peano4::grid::GridControlEvent::operator<(const GridControlEvent& other) const noexcept {
  if (refinementControl != other.refinementControl) {
    return refinementControl < other.refinementControl;
  }
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (targetH[d] != other.targetH[d]) {
      return targetH[d] < other.targetH[d];
    }
  }
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (offset[d] != other.offset[d]) {
      return offset[d] < other.offset[d];
    }
  }
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (width[d] != other.width[d]) {
      return width[d] < other.width[d];
    }
  }
  return false;
}

/**
 * Exact equality over every field.
 *
 * A caller compares a freshly merged event set against the set the mesh already realises, so
 * it can drop a resubmission that cannot change the grid. Both sets leave the same
 * deterministic pipeline, so equal events agree bitwise. A tolerance would
 * disagree with the exact ordering operator< induces, and two events could then be neither
 * less than nor equal to each other.
 */
bool peano4::grid::GridControlEvent::operator==(const GridControlEvent& other) const noexcept {
  return refinementControl == other.refinementControl and offset == other.offset and width == other.width
    and targetH == other.targetH;
}

namespace {
  void mergeEvents(std::vector<peano4::grid::GridControlEvent>& events, bool fuseEraseEvents) {
    if (events.empty()) {
      return;
    }

    std::vector<peano4::grid::GridControlEvent> refineEvents;
    std::vector<peano4::grid::GridControlEvent> eraseEvents;
    refineEvents.reserve(events.size());

    for (const auto& p : events) {
      if (p.getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Erase) {
        eraseEvents.push_back(p);
      } else if (p.getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Refine) {
        refineEvents.push_back(p);
      } else {
        assertion(false);
      }
    }

    mergeAdjacentRefinementEvents(refineEvents);
    if (fuseEraseEvents) {
      mergeAdjacentRefinementEvents(eraseEvents);
    } else {
      std::sort(eraseEvents.begin(), eraseEvents.end());
      eraseEvents.erase(std::unique(eraseEvents.begin(), eraseEvents.end()), eraseEvents.end());
    }

    events.clear();
    events.reserve(refineEvents.size() + eraseEvents.size());
    events.insert(events.end(), refineEvents.begin(), refineEvents.end());
    events.insert(events.end(), eraseEvents.begin(), eraseEvents.end());
  }
} // namespace

/**
 * Merge a set of refinement/erase instructions from an arbitrary observer.
 *
 * The merge is a pure optimisation: it shrinks the number of events every cell has to test
 * against and never decides what the grid does. Refine events are fused whenever their union
 * already is a single box: the covered point set is unchanged, and the refine rule tests
 * whether a cell overlaps some event, which the point set determines on its own.
 *
 * Erase events are only deduplicated. Their rule tests whether a cell lies inside one single
 * event, and that verdict does not survive fusion of arbitrary boxes: a cell can lie inside
 * the union of two touching boxes yet cross their seam, so fusing would erase a cell no
 * original event contains. Callers whose erase boxes are whole cells of one grid level can
 * use mergeCellAlignedEvents() to fuse them as well.
 */
void peano4::grid::GridControlEvent::merge(std::vector<GridControlEvent>& events) { mergeEvents(events, false); }

/**
 * Merge instructions whose erase events are whole cells of the level they coarsen.
 *
 * Under that precondition erase events fuse safely: every cell the erase rule tests lives on
 * the same lattice as the event boxes, so a cell inside a fused run of lattice cells is one
 * of the cells that formed the run and the containment verdict is unchanged. A caller
 * establishes the precondition by emitting one box per parent cell. For arbitrary boxes use
 * merge().
 */
void peano4::grid::GridControlEvent::mergeCellAlignedEvents(std::vector<GridControlEvent>& events) {
  mergeEvents(events, true);
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridControlEvent& event) {
  return (out << event.toString());
}

std::string peano4::grid::GridControlEvent::toString() const {
  auto rc2str = [](RefinementControl rc) { return (rc == RefinementControl::Refine) ? "Refine" : "Erase"; };

  std::ostringstream out;
  out << "GridControlEvent{";
  out << "RefinementControl=" << rc2str(getRefinementControl());

  out << ", Offset=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << offset[d];
  }
  out << "]";

  out << ", Width=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << width[d];
  }
  out << "]";

  out << ", TargetH=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ',';
    }
    out << std::setprecision(6) << targetH[d];
  }
  out << "]}";
  return out.str();
}
