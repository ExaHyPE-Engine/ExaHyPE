// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshotTest.h"

#include "peano4/Globals.h"
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
  validateEquals(runner::boundedFullyRefinedLeaves(0), std::uint64_t{1});
  validateEquals(runner::boundedFullyRefinedLeaves(1), static_cast<std::uint64_t>(THREE_POWER_D));
  validateEquals(
    runner::boundedFullyRefinedLeaves(2),
    static_cast<std::uint64_t>(THREE_POWER_D) * static_cast<std::uint64_t>(THREE_POWER_D)
  );

  auto expectedLevelTenLeaves = std::uint64_t{1};
  for (int level = 0; level < 10; ++level) {
    expectedLevelTenLeaves *= static_cast<std::uint64_t>(THREE_POWER_D);
  }
  validate(expectedLevelTenLeaves > static_cast<std::uint64_t>(std::numeric_limits<int>::max()));
  validateEquals(runner::boundedFullyRefinedLeaves(10), expectedLevelTenLeaves);
}

void runner::tests::GridSnapshotTest::testBoundedFullyRefinedLeavesHandlesLimits() {
  constexpr int MaximumSupportedExponent = 40;
  constexpr int MaximumExactLevel        = MaximumSupportedExponent / DIMENSIONS;
  auto          maximumExactLeaves       = std::uint64_t{1};
  for (int exponent = 0; exponent < DIMENSIONS * MaximumExactLevel; ++exponent) {
    maximumExactLeaves *= 3;
  }

  validateEquals(runner::boundedFullyRefinedLeaves(MaximumExactLevel), maximumExactLeaves);
  validateEquals(runner::boundedFullyRefinedLeaves(MaximumExactLevel + 1), std::numeric_limits<std::uint64_t>::max());
  validateEquals(
    runner::boundedFullyRefinedLeaves(std::numeric_limits<int>::max()),
    std::numeric_limits<std::uint64_t>::max()
  );
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
