// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEvent.h"

#include "tarch/services/ServiceRepository.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static_assert(std::is_standard_layout_v<peano4::grid::GridControlEvent>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridControlEvent>);

// static_assert(std::is_trivially_default_constructible_v<peano4::grid::GridControlEvent>);
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
   * @return true, with the fused box in @p merged, if the pair forms a single box.
   */
  bool tryMergeIntoSingleBox(
    const peano4::grid::GridControlEvent& a,
    const peano4::grid::GridControlEvent& b,
    peano4::grid::GridControlEvent&       merged
  ) {
    if (a.refinementControl != b.refinementControl) {
      return false;
    }
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
   * We sort once by (targetH, offset), which places a level's boxes contiguously and lines up
   * face-touching neighbours, then repeatedly sweep the vector, fusing each event into the
   * previously kept box whenever tryMergeIntoSingleBox() succeeds. A solid region collapses axis
   * by axis - runs along the last axis first, then the resulting columns, then the slabs - over
   * about DIMENSIONS sweeps. Fusion keeps the (already smallest) offset of the box it extends, so
   * the vector stays sorted and no further sort is needed; we stop once a sweep fuses nothing.
   * Every sweep preserves the covered region.
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
 * Exact lexicographic ordering by targetH, then offset.
 *
 * This is the order merge() sorts with: events of one refinement level (identical targetH) end
 * up contiguous, and within a level face-touching boxes become neighbours, so the greedy sweep
 * can fuse them. The comparisons are exact (==, <) on purpose.
 */
bool peano4::grid::GridControlEvent::operator<(const GridControlEvent& other) const noexcept {
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
  return false;
}

/**
 * Merge a set of refinement/erase instructions into as few boxes as possible.
 *
 * For erase events the merge is also needed for correctness: an erase only fires when a cell lies fully
 * inside an event, so two adjacent erases have to be fused into their covering box, else a
 * cell straddling their shared face would never be erased. Refine and erase events are
 * coalesced separately - their unions are unrelated and the evaluation treats them differently
 * (overlap for refine, containment for erase). The merge is exact and idempotent.
 */
void peano4::grid::GridControlEvent::merge(std::vector<GridControlEvent>& events) {
  if (events.empty()) {
    return;
  }

  std::vector<peano4::grid::GridControlEvent> refineEvents;
  std::vector<peano4::grid::GridControlEvent> eraseEvents;
  refineEvents.reserve(events.size());

  for (const auto& p : events) {
    if (p.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      eraseEvents.push_back(p);
    } else if (p.getRefinementControl() == GridControlEvent::RefinementControl::Refine) {
      refineEvents.push_back(p);
    } else {
      assertion(false);
    }
  }

  mergeAdjacentRefinementEvents(refineEvents);
  mergeAdjacentRefinementEvents(eraseEvents);

  events.clear();
  events.reserve(refineEvents.size() + eraseEvents.size());
  events.insert(events.end(), refineEvents.begin(), refineEvents.end());
  events.insert(events.end(), eraseEvents.begin(), eraseEvents.end());
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
