// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEventTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

peano4::grid::tests::GridControlEventTest::GridControlEventTest() :
  TestCase("peano4::grid::tests::GridControlEventTest") {}

void peano4::grid::tests::GridControlEventTest::run() {
  testMethod(testOperatorEquals);
  testMethod(testOperatorLess);
  testMethod(testSort);
  testMethod(testSortDeterministicGrid);
  testMethod(testMergeExactTouchVersusGap);
  testMethod(testMergeAdjacency);
  testMethod(testMergeEraseAdjacency);
  testMethod(testMergeEraseDoesNotOverMergeLShape);
  testMethod(testMergeDoesNotChangeGridDecisions);
  testMethod(testMergeEraseChainCollapsesToOne);
  testMethod(testMergeEraseSiblingGroup);
  testMethod(testMergeKeepsRefineAndEraseSeparate);
  testMethod(testMergeRowAndBlockCollapseToOne);
  testMethod(testMergeDoesNotMergeDiagonalOrDifferentTargetH);
  testMethod(testMergeUnionIsPreserved);
  testMethod(testMerge1);
  testMethod(testMerge2);
  testMethod(testMerge3);
  testMethod(testMPI);
}

void peano4::grid::tests::GridControlEventTest::testOperatorEquals() {
  GridControlEvent reference;
  reference.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  for (int d = 0; d < DIMENSIONS; ++d) {
    reference.offset[d]  = 0.1 * (d + 1);
    reference.width[d]   = 0.2 * (d + 1);
    reference.targetH[d] = 0.05 * (d + 1);
  }

  GridControlEvent other = reference;
  validateTrue(reference == other);

  other = reference;
  other.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  validateTrue(not(reference == other));

  other = reference;
  other.offset[0] += 1e-8;
  validateTrue(not(reference == other));

  other = reference;
  other.width[0] += 1e-8;
  validateTrue(not(reference == other));

  other = reference;
  other.targetH[0] += 1e-8;
  validateTrue(not(reference == other));

  // Equality is exact, so it agrees with the ordering: two events that compare equal are
  // neither less than one another, and a difference of a single bit separates them.
  other           = reference;
  other.offset[0] = std::nextafter(reference.offset[0], 1.0);
  validateTrue(not(reference == other));
  validateTrue(reference < other or other < reference);

  other = reference;
  validateTrue(reference == other);
  validateTrue(not(reference < other) and not(other < reference));
}

void peano4::grid::tests::GridControlEventTest::testOperatorLess() {
  {
    GridControlEvent a, b;
    a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    b.setRefinementControl(GridControlEvent::RefinementControl::Refine);

    // Equal targetH, different offsets -> compared lexicographically by offset.
    for (int d = 0; d < DIMENSIONS; ++d) {
      a.targetH[d] = 0.1;
      b.targetH[d] = 0.1;
      a.offset[d]  = 0.0;
      b.offset[d]  = 0.0;
    }

    // Make b larger along the first offset axis -> a < b.
    a.offset[0] = -0.5;
    b.offset[0] = -0.4;
    validateTrue(a < b);
    validateTrue(not(b < a));
  }

  // Different targetH -> ordered lexicographically by the targetH components. The leading axis
  // decides even when the other components point the other way, giving a strict, antisymmetric order.
  {
    GridControlEvent a, b;
    a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    b.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      a.targetH[d] = 0.5; // a has the larger trailing components
      b.targetH[d] = 0.1;
      a.offset[d]  = 0.0;
      b.offset[d]  = 0.0;
    }
    a.targetH[0] = 0.1;
    b.targetH[0] = 0.2;
    validateTrue(a < b); // decided by the leading targetH axis: 0.1 < 0.2
    validateTrue(not(b < a));
  }

  // Identical boxes -> not less than
  {
    GridControlEvent x, y;
    x.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    y = x;
    validateTrue(not(x < y));
    validateTrue(not(y < x));
  }

  // The order is total: refinement control leads, and width breaks the remaining ties, so
  // geometrically distinct events never compare equal and the sorted sequence cannot depend
  // on the order in which trees or ranks contributed the events.
  {
    GridControlEvent reference;
    reference.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      reference.offset[d]  = 0.0;
      reference.width[d]   = 0.25;
      reference.targetH[d] = 0.125;
    }

    GridControlEvent erase = reference;
    erase.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    validateTrue(reference < erase);
    validateTrue(not(erase < reference));

    GridControlEvent wider = reference;
    wider.width[0]         = 0.5;
    validateTrue(reference < wider);
    validateTrue(not(wider < reference));

    GridControlEvent anisotropicA = reference;
    GridControlEvent anisotropicB = reference;
    anisotropicA.targetH[0]       = 0.125;
    anisotropicA.targetH[1]       = 0.25;
    anisotropicB.targetH[0]       = 0.25;
    anisotropicB.targetH[1]       = 0.125;
#if DIMENSIONS == 3
    anisotropicA.targetH[2] = 0.5;
    anisotropicB.targetH[2] = 0.5;
#endif
    validateTrue(anisotropicA < anisotropicB);
    validateTrue(not(anisotropicB < anisotropicA));

    const std::vector<GridControlEvent> events{reference, erase, wider, anisotropicA, anisotropicB};
    for (std::size_t i = 0; i < events.size(); ++i) {
      for (std::size_t j = 0; j < events.size(); ++j) {
        validateTrue(i == j or not(events[i] < events[j] and events[j] < events[i]));
      }
    }
  }
}

void peano4::grid::tests::GridControlEventTest::testSortDeterministicGrid() {
  // Build several boxes with same volume but arranged to test lexicographic offset sort.
  std::vector<GridControlEvent> events;
  events.reserve(8);

  for (int i = 0; i < 8; ++i) {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      e.targetH[d] = 0.1;
      e.width[d]   = 0.2;
      // Offsets chosen so sorting by offset[0], then offset[1], ...
      if (d == 0) {
        e.offset[d] = (i % 4) * 0.1 - 0.3; // -0.3, -0.2, -0.1, 0.0 repeating
      } else if (d == 1) {
        e.offset[d] = (i / 4) * 0.1 - 0.1; // -0.1 for first 4, 0.0 for next 4
      }
#if DIMENSIONS == 3
      else {
        e.offset[d] = 0.0;
      }
#endif
    }
    events.push_back(e);
  }

  std::sort(events.begin(), events.end());

  // After sorting first half should have offset[0] == -0.3 or -0.2 etc.
  validateEquals(events.size(), 8u);
  // Ensure monotonic non-decreasing offsets lexicographically along axis 0 then 1
  for (std::size_t i = 1; i < events.size(); ++i) {
    bool ok = false;
    for (int d = 0; d < DIMENSIONS; ++d) {
      if (tarch::la::smaller(events[i - 1].offset[d], events[i].offset[d])) {
        ok = true;
        break;
      }
      if (tarch::la::greater(events[i - 1].offset[d], events[i].offset[d])) {
        ok = false;
        break;
      }
      // else equal -> continue to next dimension
    }
    validateTrue(ok);
  }
}

void peano4::grid::tests::GridControlEventTest::testMergeExactTouchVersusGap() {
  // The merge is exact. Two boxes that are face-aligned and touch (or overlap) fuse into one;
  // the same two boxes with a genuine gap between them stay separate.
  auto makeBox = [](double x0) {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      e.offset[d]  = 0.0;
      e.width[d]   = 0.5;
      e.targetH[d] = 0.1;
    }
    e.offset[0] = x0;
    return e;
  };

  // a:[0,0.5], b exactly touching at x=0.5 -> single box [0,1].
  {
    std::vector<GridControlEvent> events{makeBox(0.0), makeBox(0.5)};
    GridControlEvent::merge(events);
    const std::string touchMsg = events.empty() ? std::string{} : events[0].toString();
    validateEqualsWithParams1(events.size(), 1u, touchMsg);
    validateTrue(tarch::la::equals(events[0].offset[0], 0.0));
    validateTrue(tarch::la::equals(events[0].width[0], 1.0));
  }

  // a:[0,0.5], b overlapping it -> single box [0,0.9].
  {
    std::vector<GridControlEvent> events{makeBox(0.0), makeBox(0.4)};
    GridControlEvent::merge(events);
    validateEquals(events.size(), 1u);
    validateTrue(tarch::la::equals(events[0].width[0], 0.9));
  }

  // a:[0,0.5], b with a genuine gap -> stays two boxes.
  {
    std::vector<GridControlEvent> events{makeBox(0.0), makeBox(0.6)};
    GridControlEvent::merge(events);
    validateEquals(events.size(), 2u);
  }
}

void peano4::grid::tests::GridControlEventTest::testMergeRowAndBlockCollapseToOne() {
  // A contiguous run of unit boxes collapses to one box, in every axis, for any DIMENSIONS.
  for (int axis = 0; axis < DIMENSIONS; ++axis) {
    std::vector<GridControlEvent> events;
    for (int i = 0; i < 5; ++i) {
      GridControlEvent e;
      e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
      for (int d = 0; d < DIMENSIONS; ++d) {
        e.offset[d]  = 0.0;
        e.width[d]   = 1.0;
        e.targetH[d] = 0.3;
      }
      e.offset[axis] = static_cast<double>(i); // 5 unit boxes touching along 'axis'
      events.push_back(e);
    }
    GridControlEvent::merge(events);
    validateEqualsWithParams1(events.size(), 1u, "axis=" + std::to_string(axis));
    validateTrue(tarch::la::equals(events[0].offset[axis], 0.0));
    validateTrue(tarch::la::equals(events[0].width[axis], 5.0));
  }

  // A full solid block of unit cells collapses to a single box regardless of input order.
  std::vector<GridControlEvent> block;
  const int                     cellsPerAxis = 3;
  for (int i = 0; i < cellsPerAxis; ++i) {
    for (int j = 0; j < cellsPerAxis; ++j) {
#if DIMENSIONS == 3
      for (int k = 0; k < cellsPerAxis; ++k)
#endif
      {
        GridControlEvent e;
        e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
        for (int d = 0; d < DIMENSIONS; ++d) {
          e.width[d]   = 1.0;
          e.targetH[d] = 0.3;
        }
        e.offset[0] = static_cast<double>(i);
        e.offset[1] = static_cast<double>(j);
#if DIMENSIONS == 3
        e.offset[2] = static_cast<double>(k);
#endif
        block.push_back(e);
      }
    }
  }
  GridControlEvent::merge(block);
  const std::string blockMsg = block.empty() ? std::string{} : block[0].toString();
  validateEqualsWithParams1(block.size(), 1u, blockMsg);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateTrue(tarch::la::equals(block[0].width[d], static_cast<double>(cellsPerAxis)));
  }
}

void peano4::grid::tests::GridControlEventTest::testMergeDoesNotMergeDiagonalOrDifferentTargetH() {
  // Diagonal neighbours differ in more than one axis: their union is not a box -> keep both.
  {
    GridControlEvent a;
    a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    GridControlEvent b;
    b.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      a.offset[d]  = 0.0;
      a.width[d]   = 1.0;
      a.targetH[d] = 0.3;
      b.width[d]   = 1.0;
      b.targetH[d] = 0.3;
      b.offset[d]  = (d < 2) ? 1.0 : 0.0; // shifted in x and y -> diagonal
    }
    std::vector<GridControlEvent> events{a, b};
    GridControlEvent::merge(events);
    validateEquals(events.size(), 2u);
  }

  // Same position, different targetH (different refinement levels) -> never merge.
  {
    GridControlEvent a;
    a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    GridControlEvent b;
    b.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      a.offset[d]  = 0.0;
      a.width[d]   = 1.0;
      a.targetH[d] = 0.3;
      b.offset[d]  = 1.0; // adjacent in x only would otherwise merge...
      if (d > 0) {
        b.offset[d] = 0.0;
      }
      b.width[d]   = 1.0;
      b.targetH[d] = 0.1; // ...but a different targetH forbids it
    }
    std::vector<GridControlEvent> events{a, b};
    GridControlEvent::merge(events);
    validateEquals(events.size(), 2u);
  }
}

void peano4::grid::tests::GridControlEventTest::testMergeUnionIsPreserved() {
  // Merging must not change which points are covered. Build a set of unit cells, merge, and check
  // a dense set of sample points has identical coverage before and after.
  std::vector<GridControlEvent> original;
  auto                          cell = [](int i, int j, int k) {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      e.width[d]   = 1.0;
      e.targetH[d] = 0.3;
    }
    e.offset[0] = static_cast<double>(i);
    e.offset[1] = static_cast<double>(j);
#if DIMENSIONS == 3
    e.offset[2] = static_cast<double>(k);
#endif
    return e;
  };
  // An L / staircase shape plus an isolated cell (forces a non-trivial, multi-box result).
  original.push_back(cell(0, 0, 0));
  original.push_back(cell(1, 0, 0));
  original.push_back(cell(2, 0, 0));
  original.push_back(cell(0, 1, 0));
  original.push_back(cell(0, 2, 0));
  original.push_back(cell(5, 5, 0));

  auto covered = [](const std::vector<GridControlEvent>& evts, const tarch::la::Vector<DIMENSIONS, double>& p) {
    for (const auto& e : evts) {
      bool inside = true;
      for (int d = 0; d < DIMENSIONS; ++d) {
        if (p[d] < e.offset[d] or p[d] > e.offset[d] + e.width[d]) {
          inside = false;
          break;
        }
      }
      if (inside) {
        return true;
      }
    }
    return false;
  };

  std::vector<GridControlEvent> merged = original;
  GridControlEvent::merge(merged);
  validateTrue(merged.size() < original.size()); // some coalescing happened

  // Dense sample of the region; coverage must be identical cell-by-cell.
  for (int i = -1; i <= 7; ++i) {
    for (int j = -1; j <= 7; ++j) {
#if DIMENSIONS == 3
      for (int k = -1; k <= 1; ++k)
#endif
      {
        tarch::la::Vector<DIMENSIONS, double> p;
        p[0] = i + 0.5;
        p[1] = j + 0.5;
#if DIMENSIONS == 3
        p[2] = k + 0.5;
#endif
        validateEqualsWithParams2(covered(merged, p), covered(original, p), i, j);
      }
    }
  }
}

void peano4::grid::tests::GridControlEventTest::testMergeAdjacency() {
  // Test that two face-touching events along x merge while a separated
  // event in the y-direction does not merge with them.
  std::vector<GridControlEvent> events;

  // Left square
  GridControlEvent leftBox;
  leftBox.setRefinementControl(GridControlEvent::RefinementControl::Refine);
#if DIMENSIONS == 2
  leftBox.offset  = {-0.1, 0.0};
  leftBox.width   = {0.2, 0.2};
  leftBox.targetH = {0.05, 0.05};
#endif
#if DIMENSIONS == 3
  leftBox.offset  = {-0.1, 0.0, 0.0};
  leftBox.width   = {0.2, 0.2, 0.2};
  leftBox.targetH = {0.05, 0.05, 0.05};
#endif
  events.push_back(leftBox);

  // Right square adjacent in x
  GridControlEvent rightBox = leftBox;
  rightBox.offset[0]        = 0.1; // Face-touch at x = 0.0
  events.push_back(rightBox);

  // A square above in y that should not merge with the combined x pair.
  // Move this in x so it sorts after the right box, which makes the left and
  // right box neighbours in the sorted sequence.
  GridControlEvent upperBox = leftBox;
  upperBox.offset[0]        = 0.2;
  upperBox.offset[1]        = 0.2 + 0.01;
  events.push_back(upperBox);

  GridControlEvent::merge(events);

  // Expect the two x-adjacent squares to have been merged -> events.size() == 2
  validateTrue(events.size() == 2);
  // One of them should have width[0] approximately 0.4 (merged)
  bool foundMerged = false;
  for (const auto& e : events) {
    if (tarch::la::equals(e.width[0], 0.4, 1e-6)) {
      validateTrue(tarch::la::equals(e.offset[0], -0.1, 1e-6));
      foundMerged = true;
      break;
    }
  }
  validateTrue(foundMerged);

  // The remaining event should still be located at y >= 0.21
  bool foundSeparated = false;
  for (const auto& e : events) {
    if (not tarch::la::equals(e.width[0], 0.4, 1e-6)) {
      // This is the unmerged one
      validateTrue(e.offset[1] >= 0.21 - 1e-12);
      foundSeparated = true;
    }
  }
  validateTrue(foundSeparated);
}

void peano4::grid::tests::GridControlEventTest::testMergeEraseAdjacency() {
  std::vector<GridControlEvent> events;

  GridControlEvent leftBox;
  leftBox.setRefinementControl(GridControlEvent::RefinementControl::Erase);
#if DIMENSIONS == 2
  leftBox.offset  = {-0.1, 0.0};
  leftBox.width   = {0.2, 0.2};
  leftBox.targetH = {0.2, 0.2};
#endif
#if DIMENSIONS == 3
  leftBox.offset  = {-0.1, 0.0, 0.0};
  leftBox.width   = {0.2, 0.2, 0.2};
  leftBox.targetH = {0.2, 0.2, 0.2};
#endif
  events.push_back(leftBox);

  GridControlEvent rightBox = leftBox;
  rightBox.offset[0]        = 0.1; // Face-touch at x = 0.0
  events.push_back(rightBox);

  GridControlEvent::mergeCellAlignedEvents(events);

  validateEquals(events.size(), 1u);
  validateEquals(
    static_cast<std::uint8_t>(events[0].getRefinementControl()),
    static_cast<std::uint8_t>(GridControlEvent::RefinementControl::Erase)
  );
  validateTrue(tarch::la::equals(events[0].offset[0], -0.1, 1e-12));
  validateTrue(tarch::la::equals(events[0].width[0], 0.4, 1e-12));
}

void peano4::grid::tests::GridControlEventTest::testMergeEraseDoesNotOverMergeLShape() {
  // Two erase boxes whose union is not a single axis-aligned box (L-shape).
  // They must not be merged into the bounding box, as that would introduce
  // erase coverage that did not exist in either original event.
  std::vector<GridControlEvent> events;

  GridControlEvent a;
  a.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  GridControlEvent b;
  b.setRefinementControl(GridControlEvent::RefinementControl::Erase);

#if DIMENSIONS == 2
  // a: [0,2]x[0,1]
  a.offset  = {0.0, 0.0};
  a.width   = {2.0, 1.0};
  a.targetH = {1.0, 1.0};
  // b: [1,2]x[0,2]
  b.offset  = {1.0, 0.0};
  b.width   = {1.0, 2.0};
  b.targetH = {1.0, 1.0};
#endif

#if DIMENSIONS == 3
  // Same L-shape in x/y, identical extent in z.
  // a: [0,2]x[0,1]x[0,1]
  a.offset  = {0.0, 0.0, 0.0};
  a.width   = {2.0, 1.0, 1.0};
  a.targetH = {1.0, 1.0, 1.0};
  // b: [1,2]x[0,2]x[0,1]
  b.offset  = {1.0, 0.0, 0.0};
  b.width   = {1.0, 2.0, 1.0};
  b.targetH = {1.0, 1.0, 1.0};
#endif

  events.push_back(a);
  events.push_back(b);

  GridControlEvent::mergeCellAlignedEvents(events);

  // Must remain two events.
  validateEquals(events.size(), 2u);
}

void peano4::grid::tests::GridControlEventTest::testMergeDoesNotChangeGridDecisions() {
  // The merge only fuses events whose union already is a box, so no cell of the grid may change
  // its mind about refining or coarsening. We replay the two rules that
  // Spacetree::evaluateGridControlEvents applies and compare every cell's verdict before and
  // after the merge. This is what lets the merge be switched off without altering the grid.
  constexpr double LeafSize      = 1.0;
  constexpr double ParentSize    = 3.0;
  constexpr int    LeavesPerAxis = 6;

  auto wantsRefine = [](
                       const std::vector<GridControlEvent>&         events,
                       const tarch::la::Vector<DIMENSIONS, double>& cellOffset,
                       const tarch::la::Vector<DIMENSIONS, double>& cellSize
                     ) {
    for (const auto& event : events) {
      const bool overlaps = tarch::la::allGreater(cellOffset + cellSize, event.offset)
        and tarch::la::allSmaller(cellOffset, event.offset + event.width);
      if (event.getRefinementControl() == GridControlEvent::RefinementControl::Refine
          and tarch::la::allSmaller(event.targetH, cellSize) and overlaps) {
        return true;
      }
    }
    return false;
  };

  auto wantsErase = [](
                      const std::vector<GridControlEvent>&         events,
                      const tarch::la::Vector<DIMENSIONS, double>& cellOffset,
                      const tarch::la::Vector<DIMENSIONS, double>& cellSize
                    ) {
    for (const auto& event : events) {
      const bool contains = tarch::la::allGreaterEquals(cellOffset, event.offset)
        and tarch::la::allSmallerEquals(cellOffset + cellSize, event.offset + event.width);
      const bool coarsensToTargetResolution = tarch::la::allGreaterEquals(event.targetH, cellSize)
        and not tarch::la::allSmallerEquals(3.0 * cellSize, event.targetH);
      if (event.getRefinementControl() == GridControlEvent::RefinementControl::Erase and coarsensToTargetResolution
          and contains) {
        return true;
      }
    }
    return false;
  };

  auto makeEvent =
    [](GridControlEvent::RefinementControl control, int i, int j, int k, double size, double targetSize) {
      GridControlEvent event;
      event.setRefinementControl(control);
      for (int d = 0; d < DIMENSIONS; ++d) {
        event.width[d]   = size;
        event.targetH[d] = targetSize;
      }
      event.offset[0] = static_cast<double>(i) * size;
      event.offset[1] = static_cast<double>(j) * size;
#if DIMENSIONS == 3
      event.offset[2] = static_cast<double>(k) * size;
#else
      static_cast<void>(k);
#endif
      return event;
    };

  // An L-shaped run of leaves to refine, and a row of parents plus a detached parent to erase.
  // Both shapes force the greedy sweep to fuse some events and to keep others apart.
  std::vector<GridControlEvent> original;
  for (int i = 0; i < 3; ++i) {
    original.push_back(makeEvent(GridControlEvent::RefinementControl::Refine, i, 0, 0, LeafSize, LeafSize / 3.0));
    original.push_back(makeEvent(GridControlEvent::RefinementControl::Refine, 0, i, 0, LeafSize, LeafSize / 3.0));
  }
  original.push_back(makeEvent(GridControlEvent::RefinementControl::Erase, 0, 0, 0, ParentSize, ParentSize));
  original.push_back(makeEvent(GridControlEvent::RefinementControl::Erase, 1, 0, 0, ParentSize, ParentSize));
  original.push_back(makeEvent(GridControlEvent::RefinementControl::Erase, 1, 1, 0, ParentSize, ParentSize));

  std::vector<GridControlEvent> merged = original;
  GridControlEvent::mergeCellAlignedEvents(merged);
  validateTrue(merged.size() < original.size());

  // Peano only ever tests cells of the grid, so we walk the leaves and their parents.
  for (const double cellSize : {LeafSize, ParentSize}) {
    const int cellsPerAxis  = static_cast<int>(static_cast<double>(LeavesPerAxis) * LeafSize / cellSize);
    int       numberOfCells = 1;
    for (int d = 0; d < DIMENSIONS; ++d) {
      numberOfCells *= cellsPerAxis;
    }

    for (int cell = 0; cell < numberOfCells; ++cell) {
      tarch::la::Vector<DIMENSIONS, double> cellOffset;
      tarch::la::Vector<DIMENSIONS, double> cellExtent;
      int                                   remainder = cell;
      for (int d = 0; d < DIMENSIONS; ++d) {
        cellOffset(d) = static_cast<double>(remainder % cellsPerAxis) * cellSize;
        cellExtent(d) = cellSize;
        remainder /= cellsPerAxis;
      }

      validateWithParams2(
        wantsRefine(merged, cellOffset, cellExtent) == wantsRefine(original, cellOffset, cellExtent),
        cellOffset,
        cellSize
      );
      validateWithParams2(
        wantsErase(merged, cellOffset, cellExtent) == wantsErase(original, cellOffset, cellExtent),
        cellOffset,
        cellSize
      );
    }
  }

  // Merging is idempotent, so re-merging a merged set cannot drift either.
  std::vector<GridControlEvent> mergedTwice = merged;
  GridControlEvent::mergeCellAlignedEvents(mergedTwice);
  validateEquals(mergedTwice.size(), merged.size());
  for (std::size_t i = 0; i < merged.size(); ++i) {
    validateTrue(mergedTwice[i] == merged[i]);
  }

  // Erase boxes that are not whole cells of the level they coarsen: two half-cell
  // boxes tile the unit cell, which lies inside their union but inside neither box.
  // The general merge() must keep them apart, so the cell's erase verdict stays
  // false; fusing them would flip it to true.
  GridControlEvent leftHalf;
  leftHalf.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  leftHalf.offset   = tarch::la::Vector<DIMENSIONS, double>(0.0);
  leftHalf.width    = tarch::la::Vector<DIMENSIONS, double>(1.0);
  leftHalf.targetH  = tarch::la::Vector<DIMENSIONS, double>(1.0);
  leftHalf.width[0] = 0.5;

  GridControlEvent rightHalf = leftHalf;
  rightHalf.offset[0]        = 0.5;

  std::vector<GridControlEvent> halfCellEvents{leftHalf, rightHalf, leftHalf};

  const tarch::la::Vector<DIMENSIONS, double> unitCellOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> unitCellExtent(1.0);
  validateTrue(not wantsErase(halfCellEvents, unitCellOffset, unitCellExtent));

  GridControlEvent::merge(halfCellEvents);

  // The duplicate is removed, the two distinct boxes survive unfused.
  validateEquals(halfCellEvents.size(), 2u);
  validateTrue(not wantsErase(halfCellEvents, unitCellOffset, unitCellExtent));
}

void peano4::grid::tests::GridControlEventTest::testMergeEraseChainCollapsesToOne() {
  // A straight run of touching erase boxes coalesces into a single erase box spanning the run,
  // keeping the erase type.
  std::vector<GridControlEvent> events;
  for (int i = 0; i < 4; ++i) {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    for (int d = 0; d < DIMENSIONS; ++d) {
      e.offset[d]  = 0.0;
      e.width[d]   = 1.0;
      e.targetH[d] = 1.0;
    }
    e.offset[0] = static_cast<double>(i);
    events.push_back(e);
  }

  GridControlEvent::mergeCellAlignedEvents(events);

  validateEquals(events.size(), 1u);
  validateEquals(
    static_cast<std::uint8_t>(events[0].getRefinementControl()),
    static_cast<std::uint8_t>(GridControlEvent::RefinementControl::Erase)
  );
  validateTrue(tarch::la::equals(events[0].offset[0], 0.0, 1e-12));
  validateTrue(tarch::la::equals(events[0].width[0], 4.0, 1e-12));
}

void peano4::grid::tests::GridControlEventTest::testMergeEraseSiblingGroup() {
  // Leaf-sized erase requests are not whole cells of the level they coarsen, so the general
  // merge() may not fuse them: a complete 3^d sibling tiling must not collapse into a
  // parent-covering box behind the back of the caller's consensus decision, and with one
  // sibling missing a parent-covering box would coarsen a cell no event asked to coarsen.
  auto makeEvent = [](const tarch::la::Vector<DIMENSIONS, double>& offset) {
    GridControlEvent event;
    event.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    event.offset  = offset;
    event.width   = tarch::la::Vector<DIMENSIONS, double>(1.0);
    event.targetH = tarch::la::Vector<DIMENSIONS, double>(3.0);
    return event;
  };

  auto containsParent = [](const GridControlEvent& event) {
    return tarch::la::allSmallerEquals(event.offset, tarch::la::Vector<DIMENSIONS, double>(0.0))
      and tarch::la::allGreaterEquals(event.offset + event.width, tarch::la::Vector<DIMENSIONS, double>(3.0));
  };

  std::vector<GridControlEvent> complete;
#if DIMENSIONS == 2
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 3; i++) {
      complete.push_back(makeEvent({static_cast<double>(i), static_cast<double>(j)}));
    }
  }
#endif
#if DIMENSIONS == 3
  for (int k = 0; k < 3; k++) {
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3; i++) {
        complete.push_back(makeEvent({static_cast<double>(i), static_cast<double>(j), static_cast<double>(k)}));
      }
    }
  }
#endif

  GridControlEvent::merge(complete);
  bool completeGroupCoversParent = false;
  for (const auto& event : complete) {
    completeGroupCoversParent |= containsParent(event);
  }
  validateTrue(not completeGroupCoversParent);

  std::vector<GridControlEvent> incomplete;
#if DIMENSIONS == 2
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 3; i++) {
      if (not(i == 1 and j == 1)) {
        incomplete.push_back(makeEvent({static_cast<double>(i), static_cast<double>(j)}));
      }
    }
  }
#endif
#if DIMENSIONS == 3
  for (int k = 0; k < 3; k++) {
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3; i++) {
        if (not(i == 1 and j == 1 and k == 1)) {
          incomplete.push_back(makeEvent({static_cast<double>(i), static_cast<double>(j), static_cast<double>(k)}));
        }
      }
    }
  }
#endif

  GridControlEvent::merge(incomplete);
  bool hasParentCoveringEvent = false;
  for (const auto& event : incomplete) {
    hasParentCoveringEvent |= containsParent(event);
  }
  validateTrue(not hasParentCoveringEvent);
}

void peano4::grid::tests::GridControlEventTest::testMergeKeepsRefineAndEraseSeparate() {
  // Refine and erase events are coalesced independently and never fused with each other, even
  // when they occupy the same region: the evaluation treats them differently (overlap for refine,
  // containment for erase), so their boxes are unrelated.
  std::vector<GridControlEvent> events;

  // A row of two refine boxes and, at the same location and targetH, a row of two erase boxes.
  for (int type = 0; type < 2; ++type) {
    for (int i = 0; i < 2; ++i) {
      GridControlEvent e;
      e.setRefinementControl(
        (type == 0) ? GridControlEvent::RefinementControl::Refine : GridControlEvent::RefinementControl::Erase
      );
      for (int d = 0; d < DIMENSIONS; ++d) {
        e.offset[d]  = 0.0;
        e.width[d]   = 1.0;
        e.targetH[d] = 1.0;
      }
      e.offset[0] = static_cast<double>(i); // two boxes touching along x
      events.push_back(e);
    }
  }

  GridControlEvent::mergeCellAlignedEvents(events);

  // Each row coalesces to one box: two events total, one refine and one erase, no cross-type fuse.
  validateEquals(events.size(), 2u);
  int refine = 0;
  int erase  = 0;
  for (const auto& e : events) {
    if (e.getRefinementControl() == GridControlEvent::RefinementControl::Refine) {
      ++refine;
    } else {
      ++erase;
    }
    validateTrue(tarch::la::equals(e.width[0], 2.0, 1e-12));
  }
  validateEquals(refine, 1);
  validateEquals(erase, 1);
}

void peano4::grid::tests::GridControlEventTest::testSort() {
  std::vector<GridControlEvent> events;
  events.reserve(10);

#if DIMENSIONS == 2
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, 0.05};
    e.width   = {0.122222, 0.122222};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, -0.0611111};
    e.width   = {0.122222, 0.233333};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, -0.0611111};
    e.width   = {0.122222, 0.455556};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, 0.161111};
    e.width   = {0.233333, 0.233333};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, 0.383333};
    e.width   = {0.455556, 0.122222};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, 0.05};
    e.width   = {0.122222, 0.455556};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, 0.383333};
    e.width   = {0.566667, 0.122222};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, -0.0611111};
    e.width   = {0.566667, 0.122222};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, -0.0611111};
    e.width   = {0.344444, 0.455556};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {0.272222, -0.505556};
    e.width   = {0.233333, 0.455556};
    e.targetH = {0.037037, 0.037037};
    events.push_back(e);
  }
#endif

#if DIMENSIONS == 3
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, 0.05, 0.272222};
    e.width   = {0.122222, 0.122222, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, -0.0611111, 0.272222};
    e.width   = {0.122222, 0.233333, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, -0.0611111, 0.383333};
    e.width   = {0.122222, 0.455556, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.172222, 0.161111, 0.272222};
    e.width   = {0.233333, 0.233333, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, 0.383333, 0.383333};
    e.width   = {0.455556, 0.122222, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, 0.05, 0.383333};
    e.width   = {0.122222, 0.455556, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, 0.383333, 0.272222};
    e.width   = {0.566667, 0.122222, 0.122222};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.0611111, -0.0611111, 0.272222};
    e.width   = {0.566667, 0.122222, 0.233333};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {-0.505556, -0.0611111, 0.272222};
    e.width   = {0.344444, 0.455556, 0.233333};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset  = {0.272222, -0.505556, 0.272222};
    e.width   = {0.233333, 0.455556, 0.233333};
    e.targetH = {0.037037, 0.037037, 0.037037};
    events.push_back(e);
  }
#endif

  std::sort(events.begin(), events.end());

  validateEquals(events.size(), 10);

  // First two events should have offset[0] == -0.505556
  validateNumericalEqualsWithParams1(events[0].offset[0], -0.505556, events[0].toString());
  validateNumericalEqualsWithParams1(events[1].offset[0], -0.505556, events[1].toString());
}

void peano4::grid::tests::GridControlEventTest::testMerge1() {
#if DIMENSIONS == 2
  std::vector<GridControlEvent> events;
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = -0.0333333;
    e.offset[1]  = -0.0333333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = -0.0333333;
    e.offset[1]  = 0.3;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = -0.0333333;
    e.offset[1]  = 0.633333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.3;
    e.offset[1]  = -0.0333333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.633333;
    e.offset[1]  = -0.0333333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.633333;
    e.offset[1]  = 0.3;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.3;
    e.offset[1]  = 0.3;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.3;
    e.offset[1]  = 0.633333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }
  {
    GridControlEvent e;
    e.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    e.offset[0]  = 0.633333;
    e.offset[1]  = 0.633333;
    e.width[0]   = 0.4;
    e.width[1]   = 0.4;
    e.targetH[0] = 0.111111;
    e.targetH[1] = 0.111111;
    events.push_back(e);
  }

  GridControlEvent::merge(events);

  validateEquals(events.size(), 1);
  validateWithParams1(tarch::la::equals(events[0].offset, -0.0333333, 0.001), events[0].toString());
  validateWithParams1(tarch::la::equals(events[0].width, 1.06667, 0.001), events[0].toString());
#endif
}

void peano4::grid::tests::GridControlEventTest::testMerge2() {
#if DIMENSIONS == 2
  GridControlEvent event0;
  event0.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event0.offset[0]  = 0.316667;
  event0.offset[1]  = 0.316667;
  event0.width[0]   = 0.366667;
  event0.width[1]   = 0.366667;
  event0.targetH[0] = 0.111111;
  event0.targetH[1] = 0.111111;

  GridControlEvent event1;
  event1.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event1.offset[0]  = 0.65;
  event1.offset[1]  = 0.316667;
  event1.width[0]   = 0.366667;
  event1.width[1]   = 0.366667;
  event1.targetH[0] = 0.111111;
  event1.targetH[1] = 0.111111;

  GridControlEvent event2;
  event2.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event2.offset[0]  = 0.65;
  event2.offset[1]  = -0.0166667;
  event2.width[0]   = 0.366667;
  event2.width[1]   = 0.366667;
  event2.targetH[0] = 0.111111;
  event2.targetH[1] = 0.111111;

  GridControlEvent event3;
  event3.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event3.offset[0]  = 0.316667;
  event3.offset[1]  = -0.0166667;
  event3.width[0]   = 0.366667;
  event3.width[1]   = 0.366667;
  event3.targetH[0] = 0.111111;
  event3.targetH[1] = 0.111111;

  GridControlEvent event4;
  event4.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event4.offset[0]  = -0.0166667;
  event4.offset[1]  = -0.0166667;
  event4.width[0]   = 0.366667;
  event4.width[1]   = 0.366667;
  event4.targetH[0] = 0.111111;
  event4.targetH[1] = 0.111111;

  std::vector<GridControlEvent> events;
  events.push_back(event0);
  events.push_back(event1);
  events.push_back(event2);
  events.push_back(event3);
  events.push_back(event4);

  GridControlEvent::merge(events);

  // The five boxes form an L-shape (a bottom row of three, a top row of two). Its union is not a
  // single rectangle, so it cannot collapse to one box. Sorted by (offset[0], offset[1]) the
  // top-left box sits between the bottom-row boxes, so the greedy sweep fuses the two bottom-left
  // boxes and the two right boxes and keeps the top-left box on its own: three boxes.
  const std::string msg = events.empty() ? std::string{} : events[0].toString();
  validateEqualsWithParams1(events.size(), 3u, msg);
#endif
}

void peano4::grid::tests::GridControlEventTest::testMerge3() {
#if DIMENSIONS == 2
  std::vector<GridControlEvent> events;

  GridControlEvent a;
  a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  a.offset[0]  = 0.65;
  a.offset[1]  = 0.65;
  a.width[0]   = 0.366667;
  a.width[1]   = 0.366667;
  a.targetH[0] = 0.111111;
  a.targetH[1] = 0.111111;
  events.push_back(a);

  GridControlEvent b;
  b.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  b.offset[0]  = 0.883333;
  b.offset[1]  = 0.883333;
  b.width[0]   = 0.122222;
  b.width[1]   = 0.122222;
  b.targetH[0] = 0.037037;
  b.targetH[1] = 0.037037;
  events.push_back(b);

  GridControlEvent::merge(events);

  validateEquals(events.size(), 2);
#endif
}

void peano4::grid::tests::GridControlEventTest::testMPI() {
  auto&     rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm  comm          = rank.getCommunicator();
  const int myRank        = rank.getRank();
  const int numberOfRanks = rank.getNumberOfRanks();
  const int tag           = 4242;

  // Build a rank-distinct control event
  GridControlEvent sentEvent;
  sentEvent.setRefinementControl(
    (myRank % 2 == 0) ? GridControlEvent::RefinementControl::Refine : GridControlEvent::RefinementControl::Erase
  );
  for (int d = 0; d < DIMENSIONS; ++d) {
    sentEvent.offset[d]  = 0.1 * (d + 1) + 0.001 * myRank;
    sentEvent.width[d]   = 0.2 + 0.01 * d + 0.0005 * myRank;
    sentEvent.targetH[d] = 0.05 * (d + 1) + 0.0001 * myRank;
  }

  GridControlEvent receivedEvent;
  MPI_Status       status;

  if (numberOfRanks == 1) {
    tarch::mpi::sendrecv(
      &sentEvent,
      sizeof(GridControlEvent),
      MPI_BYTE,
      myRank,
      tag,
      &receivedEvent,
      sizeof(GridControlEvent),
      MPI_BYTE,
      myRank,
      tag,
      comm,
      &status
    );
  } else {
    const int destination = (myRank + 1) % numberOfRanks;
    const int source      = (myRank - 1 + numberOfRanks) % numberOfRanks;

    if ((myRank % 2) == 0) {
      // Even ranks send then receive
      tarch::mpi::send(&sentEvent, sizeof(GridControlEvent), MPI_BYTE, destination, tag, comm);
      tarch::mpi::receive(&receivedEvent, sizeof(GridControlEvent), MPI_BYTE, source, tag, comm, &status);
    } else {
      // Odd ranks receive then send
      tarch::mpi::receive(&receivedEvent, sizeof(GridControlEvent), MPI_BYTE, source, tag, comm, &status);
      tarch::mpi::send(&sentEvent, sizeof(GridControlEvent), MPI_BYTE, destination, tag, comm);
    }
  }

  const int  expectedSenderRank        = (numberOfRanks == 1) ? myRank : ((myRank - 1 + numberOfRanks) % numberOfRanks);
  const auto expectedRefinementControl = (expectedSenderRank % 2 == 0) ? GridControlEvent::RefinementControl::Refine
                                                                       : GridControlEvent::RefinementControl::Erase;
  validateEquals(
    static_cast<std::uint8_t>(receivedEvent.getRefinementControl()),
    static_cast<std::uint8_t>(expectedRefinementControl)
  );

  for (int d = 0; d < DIMENSIONS; ++d) {
    const double expectedOffset  = 0.1 * (d + 1) + 0.001 * expectedSenderRank;
    const double expectedWidth   = 0.2 + 0.01 * d + 0.0005 * expectedSenderRank;
    const double expectedTargetH = 0.05 * (d + 1) + 0.0001 * expectedSenderRank;

    validateTrue(std::fabs(receivedEvent.offset[d] - expectedOffset) < 1e-12);
    validateTrue(std::fabs(receivedEvent.width[d] - expectedWidth) < 1e-12);
    validateTrue(std::fabs(receivedEvent.targetH[d] - expectedTargetH) < 1e-12);
  }

  validateEquals(status.MPI_SOURCE, expectedSenderRank);
}
