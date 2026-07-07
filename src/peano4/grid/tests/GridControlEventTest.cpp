// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEventTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

peano4::grid::tests::GridControlEventTest::GridControlEventTest() :
  TestCase("peano4::grid::tests::GridControlEventTest") {}

void peano4::grid::tests::GridControlEventTest::run() {
  testMethod(testOperatorLess);
  testMethod(testSort);
  testMethod(testSortDeterministicGrid);
  testMethod(testMergeExactTouchVersusGap);
  testMethod(testMergeAdjacency);
  testMethod(testMergeEraseAdjacency);
  testMethod(testMergeEraseDoesNotOverMergeLShape);
  testMethod(testMergeEraseEnablesStraddlingErase);
  testMethod(testMergeEraseChainCollapsesToOne);
  testMethod(testMergeKeepsRefineAndEraseSeparate);
  testMethod(testMergeRowAndBlockCollapseToOne);
  testMethod(testMergeDoesNotMergeDiagonalOrDifferentTargetH);
  testMethod(testMergeUnionIsPreserved);
  testMethod(testMerge1);
  testMethod(testMerge2);
  testMethod(testMerge3);
  testMethod(testMPI);
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
  const int                     N = 3;
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
#if DIMENSIONS == 3
      for (int k = 0; k < N; ++k)
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
    validateTrue(tarch::la::equals(block[0].width[d], static_cast<double>(N)));
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
  GridControlEvent L;
  L.setRefinementControl(GridControlEvent::RefinementControl::Refine);
#if DIMENSIONS == 2
  L.offset  = {-0.1, 0.0};
  L.width   = {0.2, 0.2};
  L.targetH = {0.05, 0.05};
#endif
#if DIMENSIONS == 3
  L.offset  = {-0.1, 0.0, 0.0};
  L.width   = {0.2, 0.2, 0.2};
  L.targetH = {0.05, 0.05, 0.05};
#endif
  events.push_back(L);

  // Right square adjacent in x
  GridControlEvent R = L;
  R.offset[0]        = 0.1; // Face-touch at x = 0.0
  events.push_back(R);

  // A square above in y that should not merge with the combined x pair
  // Move this in x so it sorts after R (ensures L and R become neighbours in sort).
  GridControlEvent U = L;
  U.offset[0]        = 0.2;
  U.offset[1]        = 0.2 + 0.01;
  events.push_back(U);

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

  GridControlEvent L;
  L.setRefinementControl(GridControlEvent::RefinementControl::Erase);
#if DIMENSIONS == 2
  L.offset  = {-0.1, 0.0};
  L.width   = {0.2, 0.2};
  L.targetH = {0.05, 0.05};
#endif
#if DIMENSIONS == 3
  L.offset  = {-0.1, 0.0, 0.0};
  L.width   = {0.2, 0.2, 0.2};
  L.targetH = {0.05, 0.05, 0.05};
#endif
  events.push_back(L);

  GridControlEvent R = L;
  R.offset[0]        = 0.1; // Face-touch at x = 0.0
  events.push_back(R);

  GridControlEvent::merge(events);

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
  a.targetH = {0.1, 0.1};
  // b: [1,2]x[0,2]
  b.offset  = {1.0, 0.0};
  b.width   = {1.0, 2.0};
  b.targetH = {0.1, 0.1};
#endif

#if DIMENSIONS == 3
  // Same L-shape in x/y, identical extent in z.
  // a: [0,2]x[0,1]x[0,1]
  a.offset  = {0.0, 0.0, 0.0};
  a.width   = {2.0, 1.0, 1.0};
  a.targetH = {0.1, 0.1, 0.1};
  // b: [1,2]x[0,2]x[0,1]
  b.offset  = {1.0, 0.0, 0.0};
  b.width   = {1.0, 2.0, 1.0};
  b.targetH = {0.1, 0.1, 0.1};
#endif

  events.push_back(a);
  events.push_back(b);

  GridControlEvent::merge(events);

  // Must remain two events.
  validateEquals(events.size(), 2u);
}

void peano4::grid::tests::GridControlEventTest::testMergeEraseEnablesStraddlingErase() {
  // An erase only fires for a cell that lies fully inside a single event (see
  // Spacetree::evaluateGridControlEvents, which tests containment for erases). Two erase boxes
  // that share a face therefore have to be fused into their covering box: a cell straddling that
  // face is contained in neither original box, but in the merged one, and must be erased.
  auto contains = [](
                    const GridControlEvent&                      e,
                    const tarch::la::Vector<DIMENSIONS, double>& cellMin,
                    const tarch::la::Vector<DIMENSIONS, double>& cellMax
                  ) {
    for (int d = 0; d < DIMENSIONS; ++d) {
      if (cellMin[d] < e.offset[d] or cellMax[d] > e.offset[d] + e.width[d]) {
        return false;
      }
    }
    return true;
  };
  auto containedInAny = [&](
                          const std::vector<GridControlEvent>&         evts,
                          const tarch::la::Vector<DIMENSIONS, double>& cellMin,
                          const tarch::la::Vector<DIMENSIONS, double>& cellMax
                        ) {
    for (const auto& e : evts) {
      if (contains(e, cellMin, cellMax)) {
        return true;
      }
    }
    return false;
  };

  // a:[0,1], b:[1,2] along x, identical in the other axes, touching at x = 1.
  GridControlEvent a;
  a.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  GridControlEvent b;
  b.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  for (int d = 0; d < DIMENSIONS; ++d) {
    a.offset[d]  = 0.0;
    a.width[d]   = 1.0;
    a.targetH[d] = 0.3;
    b.offset[d]  = 0.0;
    b.width[d]   = 1.0;
    b.targetH[d] = 0.3;
  }
  b.offset[0] = 1.0; // touches a at x = 1

  // A cell straddling the shared face x = 1, well inside the box in every other axis.
  tarch::la::Vector<DIMENSIONS, double> cellMin;
  tarch::la::Vector<DIMENSIONS, double> cellMax;
  for (int d = 0; d < DIMENSIONS; ++d) {
    cellMin[d] = 0.25;
    cellMax[d] = 0.75;
  }
  cellMin[0] = 0.75;
  cellMax[0] = 1.25;

  std::vector<GridControlEvent> original{a, b};
  validateFalse(containedInAny(original, cellMin, cellMax));

  std::vector<GridControlEvent> merged = original;
  GridControlEvent::merge(merged);

  validateEquals(merged.size(), 1u);
  validateTrue(containedInAny(merged, cellMin, cellMax));
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
      e.targetH[d] = 0.3;
    }
    e.offset[0] = static_cast<double>(i);
    events.push_back(e);
  }

  GridControlEvent::merge(events);

  validateEquals(events.size(), 1u);
  validateEquals(
    static_cast<std::uint8_t>(events[0].getRefinementControl()),
    static_cast<std::uint8_t>(GridControlEvent::RefinementControl::Erase)
  );
  validateTrue(tarch::la::equals(events[0].offset[0], 0.0, 1e-12));
  validateTrue(tarch::la::equals(events[0].width[0], 4.0, 1e-12));
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
        e.targetH[d] = 0.3;
      }
      e.offset[0] = static_cast<double>(i); // two boxes touching along x
      events.push_back(e);
    }
  }

  GridControlEvent::merge(events);

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
#ifdef PEANO_WITH_MPI
  auto&     rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm  comm          = rank.getCommunicator();
  const int me            = rank.getRank();
  const int numberOfRanks = rank.getNumberOfRanks();
  const int tag           = 4242;

  // Build a rank-distinct control event
  GridControlEvent sendE;
  sendE.setRefinementControl(
    (me % 2 == 0) ? GridControlEvent::RefinementControl::Refine : GridControlEvent::RefinementControl::Erase
  );
  for (int d = 0; d < DIMENSIONS; ++d) {
    sendE.offset[d]  = 0.1 * (d + 1) + 0.001 * me;
    sendE.width[d]   = 0.2 + 0.01 * d + 0.0005 * me;
    sendE.targetH[d] = 0.05 * (d + 1) + 0.0001 * me;
  }

  GridControlEvent recvE;
  MPI_Status       status;

  if (numberOfRanks == 1) {
    tarch::mpi::sendrecv(
      &sendE,
      sizeof(GridControlEvent),
      MPI_BYTE,
      me,
      tag,
      &recvE,
      sizeof(GridControlEvent),
      MPI_BYTE,
      me,
      tag,
      comm,
      &status
    );
  } else {
    const int dest = (me + 1) % numberOfRanks;
    const int src  = (me - 1 + numberOfRanks) % numberOfRanks;

    if ((me % 2) == 0) {
      // Even ranks send then receive
      tarch::mpi::send(&sendE, sizeof(GridControlEvent), MPI_BYTE, dest, tag, comm);
      tarch::mpi::receive(&recvE, sizeof(GridControlEvent), MPI_BYTE, src, tag, comm, &status);
    } else {
      // Odd ranks receive then send
      tarch::mpi::receive(&recvE, sizeof(GridControlEvent), MPI_BYTE, src, tag, comm, &status);
      tarch::mpi::send(&sendE, sizeof(GridControlEvent), MPI_BYTE, dest, tag, comm);
    }
  }

  const int  expectedSenderRank = (numberOfRanks == 1) ? me : ((me - 1 + numberOfRanks) % numberOfRanks);
  const auto expectedRC         = (expectedSenderRank % 2 == 0) ? GridControlEvent::RefinementControl::Refine
                                                                : GridControlEvent::RefinementControl::Erase;
  validateEquals(static_cast<std::uint8_t>(recvE.getRefinementControl()), static_cast<std::uint8_t>(expectedRC));

  for (int d = 0; d < DIMENSIONS; ++d) {
    const double expOffset = 0.1 * (d + 1) + 0.001 * expectedSenderRank;
    const double expWidth  = 0.2 + 0.01 * d + 0.0005 * expectedSenderRank;
    const double expH      = 0.05 * (d + 1) + 0.0001 * expectedSenderRank;

    validateTrue(std::fabs(recvE.offset[d] - expOffset) < 1e-12);
    validateTrue(std::fabs(recvE.width[d] - expWidth) < 1e-12);
    validateTrue(std::fabs(recvE.targetH[d] - expH) < 1e-12);
  }

  validateEquals(status.MPI_SOURCE, expectedSenderRank);
#endif
}
