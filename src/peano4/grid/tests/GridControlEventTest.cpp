// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridControlEventTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "tarch/mpi/Rank.h"

peano4::grid::tests::GridControlEventTest::GridControlEventTest():
  TestCase("peano4::grid::tests::GridControlEventTest") {}

void peano4::grid::tests::GridControlEventTest::run() {
  testMethod(testOperatorLess);
  testMethod(testSort);
  testMethod(testSortDeterministicGrid);
  testMethod(testMergeTolerance);
  testMethod(testMergeAdjacency);
  testMethod(testMergeEraseAdjacency);
  testMethod(testMergeEraseDoesNotOverMergeLShape);
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

    // Same volume (product of targetH), different offsets -> compare offsets lexicographically
    for (int d = 0; d < DIMENSIONS; ++d) {
      a.targetH[d] = 0.1;
      b.targetH[d] = 0.1;
      a.offset[d]  = 0.0;
      b.offset[d]  = 0.0;
    }

    // Make b slightly larger along first axis offset -> a < b
    a.offset[0] = -0.5;
    b.offset[0] = -0.4;
    validateTrue(a < b);
    validateTrue(not(b < a));
  }

  // Different volumes -> smaller volume sorts first
  {
    GridControlEvent small, large;
    small.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    large.setRefinementControl(GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      small.targetH[d] = 0.05;
      large.targetH[d] = 0.2;
      small.offset[d]  = 0.0;
      large.offset[d]  = 0.0;
    }
    validateTrue(small < large);
    validateTrue(not(large < small));
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
      if (d == 0)
        e.offset[d] = (i % 4) * 0.1 - 0.3; // -0.3, -0.2, -0.1, 0.0 repeating
      else if (d == 1)
        e.offset[d] = (i / 4) * 0.1 - 0.1; // -0.1 for first 4, 0.0 for next 4
#if DIMENSIONS == 3
      else
        e.offset[d] = 0.0;
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

void peano4::grid::tests::GridControlEventTest::testMergeTolerance() {
#if DIMENSIONS == 2
  // Two events that would be just outside tolerance when merging, and just inside tolerance.
  std::vector<GridControlEvent> events;

  GridControlEvent a, b;
  a.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  b.setRefinementControl(GridControlEvent::RefinementControl::Refine);

  a.offset[0]  = 0.0;
  a.offset[1]  = 0.0;
  a.width[0]   = 0.5;
  a.width[1]   = 0.5;
  a.targetH[0] = 0.1;
  a.targetH[1] = 0.1;

  // b touches a on the face but with a tiny gap relative to tolerance
  const double tinyGap = 1e-6;
  b.offset[0]          = 0.5 + tinyGap;
  b.offset[1]          = 0.0;
  b.width[0]           = 0.5;
  b.width[1]           = 0.5;
  b.targetH[0]         = 0.1;
  b.targetH[1]         = 0.1;

  // First with a tiny tolerance -> should NOT merge
  events.clear();
  events.push_back(a);
  events.push_back(b);
  GridControlEvent::merge(events, 1e-8);
  validateTrue(events.size() == 2);

  // With a more relaxed tolerance -> should merge (face-touch within relative tolerance)
  events.clear();
  events.push_back(a);
  events.push_back(b);
  GridControlEvent::merge(events, 1e-2);
  validateTrue(events.size() == 1);
#endif
}

void peano4::grid::tests::GridControlEventTest::testMergeAdjacency() {
  // Test that two face-touching events along x merge while a separated
  // event in the y-direction does not merge with them.
  constexpr double              Tolerance = 0.1;
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

  // A square above in y that should NOT merge with the combined x pair
  // Move this in x so it sorts after R (ensures L and R become neighbours in sort).
  GridControlEvent U = L;
  U.offset[0]        = 0.2;
  U.offset[1]        = 0.2 + 0.01;
  events.push_back(U);

  GridControlEvent::merge(events, Tolerance);

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
  constexpr double              Tolerance = 0.1;
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

  GridControlEvent::merge(events, Tolerance);

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

  GridControlEvent::merge(events, 0.0);

  // Must remain two events.
  validateEquals(events.size(), 2u);
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
  constexpr double Tolerance = 0.1;

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

  GridControlEvent::merge(events, Tolerance);

  validateEquals(events.size(), 1);
  validateWithParams1(tarch::la::equals(events[0].offset, -0.0333333, 0.001), events[0].toString());
  validateWithParams1(tarch::la::equals(events[0].width, 1.06667, 0.001), events[0].toString());
#endif
}

void peano4::grid::tests::GridControlEventTest::testMerge2() {
#if DIMENSIONS == 2
  constexpr double Tolerance = 0.1;

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

  GridControlEvent::merge(events, Tolerance);

  validate(events.size() > 1);
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

  GridControlEvent::merge(events, 0.4);

  validateEquals(events.size(), 2);
#endif
}

void peano4::grid::tests::GridControlEventTest::testMPI() {
#ifdef WITH_MPI
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
    MPI_Sendrecv(
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
      MPI_Send(&sendE, sizeof(GridControlEvent), MPI_BYTE, dest, tag, comm);
      MPI_Recv(&recvE, sizeof(GridControlEvent), MPI_BYTE, src, tag, comm, &status);
    } else {
      // Odd ranks receive then send
      MPI_Recv(&recvE, sizeof(GridControlEvent), MPI_BYTE, src, tag, comm, &status);
      MPI_Send(&sendE, sizeof(GridControlEvent), MPI_BYTE, dest, tag, comm);
    }
  }

  const int  expectedSenderRank = (numberOfRanks == 1) ? me : ((me - 1 + numberOfRanks) % numberOfRanks);
  const auto expectedRC         = (expectedSenderRank % 2 == 0)
                                    ? GridControlEvent::RefinementControl::Refine
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
