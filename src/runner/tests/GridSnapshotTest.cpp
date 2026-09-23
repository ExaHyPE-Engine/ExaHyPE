// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshotTest.h"

#include "peano4/sfc/ActiveCurve.h"
#include "runner/Format.h"
#include "runner/GridSnapshot.h"

runner::tests::GridSnapshotTest::GridSnapshotTest() :
  tarch::tests::TestCase("runner::tests::GridSnapshotTest") {}

void runner::tests::GridSnapshotTest::run() {
  testMethod(testBoundedFullyRefinedLeavesGrowsPerLevel);
  testMethod(testBoundedFullyRefinedLeavesHandlesLimits);
  testMethod(testGridCellSummary);
  testMethod(testDescribeGridSnapshotRendersFilledRows);
  testMethod(testDescribeGridTransitionRendersChangedAndMemoryRows);
}

void runner::tests::GridSnapshotTest::testBoundedFullyRefinedLeavesGrowsPerLevel() {
  constexpr auto ChildrenPerCell = static_cast<std::uint64_t>(peano4::sfc::ActiveCurve::ChildrenPerCell);
  validateEquals(runner::boundedFullyRefinedLeaves(0), std::uint64_t{1});
  validateEquals(runner::boundedFullyRefinedLeaves(1), ChildrenPerCell);
  validateEquals(runner::boundedFullyRefinedLeaves(2), ChildrenPerCell * ChildrenPerCell);

  // The count grows beyond the 32-bit range on some level and keeps its exact value there.
  auto expectedLeaves = std::uint64_t{1};
  int  level          = 0;
  while (expectedLeaves <= static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    expectedLeaves *= ChildrenPerCell;
    level++;
  }
  validateEquals(runner::boundedFullyRefinedLeaves(level), expectedLeaves);
}

void runner::tests::GridSnapshotTest::testBoundedFullyRefinedLeavesHandlesLimits() {
  constexpr auto SubdivisionFactor = static_cast<std::uint64_t>(peano4::sfc::ActiveCurve::SubdivisionFactor);
  constexpr auto MaximumCount      = std::numeric_limits<std::uint64_t>::max();

  // The largest power of the subdivision factor a 64-bit count holds.
  auto maximumExactPower    = std::uint64_t{1};
  int  maximumExactExponent = 0;
  while (maximumExactPower <= MaximumCount / SubdivisionFactor) {
    maximumExactPower *= SubdivisionFactor;
    maximumExactExponent++;
  }

  const int maximumExactLevel  = maximumExactExponent / DIMENSIONS;
  auto      maximumExactLeaves = std::uint64_t{1};
  for (int exponent = 0; exponent < DIMENSIONS * maximumExactLevel; exponent++) {
    maximumExactLeaves *= SubdivisionFactor;
  }

  validateEquals(runner::boundedFullyRefinedLeaves(maximumExactLevel), maximumExactLeaves);
  validateEquals(runner::boundedFullyRefinedLeaves(maximumExactLevel + 1), MaximumCount);
  validateEquals(runner::boundedFullyRefinedLeaves(std::numeric_limits<int>::max()), MaximumCount);
}

void runner::tests::GridSnapshotTest::testGridCellSummary() {
  validateEquals(runner::gridCellSummary(81, 10), std::string("81  (Cells 91)"));
  validateEquals(runner::gridCellSummary(1234, 5678), std::string("1,234  (Cells 6,912)"));
  validateEquals(
    runner::gridCellSummary(std::numeric_limits<std::uint64_t>::max(), 1),
    std::string("18,446,744,073,709,551,615  (Cells 18,446,744,073,709,551,615)")
  );
}

/** A filled row renders as value and bracketed detail under its section; an empty row stays out. */
void runner::tests::GridSnapshotTest::testDescribeGridSnapshotRendersFilledRows() {
  runner::GridSnapshot snapshot;
  snapshot.entries[runner::GridSnapshot::GridLeaves] = {"81", "Cells 91"};
  snapshot.entries[runner::GridSnapshot::GridLevels] = {"2 .. 4", ""};

  const auto lines = runner::describeGridSnapshot(snapshot);
  validateEquals(lines.size(), std::size_t{3});
  validateTrue(lines[0].find("Grid") != std::string::npos);
  validateTrue(lines[1].find("Number of Leaves") != std::string::npos);
  validateTrue(lines[1].find("81  (Cells 91)") != std::string::npos);
  validateTrue(lines[2].find("Grid Levels") != std::string::npos);
  validateTrue(lines[2].find("2 .. 4") != std::string::npos);
  validateTrue(lines[2].find('(') == std::string::npos);
}

/**
 * A changed row carries both values, an unchanged row stays out, and the
 * memory rows stay in without an arrow.
 */
void runner::tests::GridSnapshotTest::testDescribeGridTransitionRendersChangedAndMemoryRows() {
  runner::GridSnapshot before;
  runner::GridSnapshot after;
  before.entries[runner::GridSnapshot::GridLeaves]       = {"81", "Cells 91"};
  after.entries[runner::GridSnapshot::GridLeaves]        = {"633", "Cells 712"};
  before.entries[runner::GridSnapshot::GridLevels]       = {"2 .. 4", ""};
  after.entries[runner::GridSnapshot::GridLevels]        = {"2 .. 4", ""};
  before.entries[runner::GridSnapshot::MemoryProcess]    = {"304.32 MiB", "Sum over Ranks"};
  after.entries[runner::GridSnapshot::MemoryProcess]     = {"304.32 MiB", "Sum over Ranks"};
  before.entries[runner::GridSnapshot::MemorySolverHeap] = {"19.55 MiB", "Cell 8.73 MiB · Face 10.82 MiB"};
  after.entries[runner::GridSnapshot::MemorySolverHeap]  = {"24.10 MiB", "Cell 10.73 MiB · Face 13.37 MiB"};

  const auto lines = runner::describeGridTransition(before, after);
  validateEquals(lines.size(), std::size_t{5});
  validateTrue(lines[1].find("81 → 633  (Cells 712)") != std::string::npos);
  for (const auto& line : lines) {
    validateTrue(line.find("Grid Levels") == std::string::npos);
  }
  validateTrue(lines[3].find("304.32 MiB  (Sum over Ranks)") != std::string::npos);
  validateTrue(lines[3].find("→") == std::string::npos);
  validateTrue(lines[4].find("19.55 MiB → 24.10 MiB  (Cell 10.73 MiB · Face 13.37 MiB)") != std::string::npos);
}
