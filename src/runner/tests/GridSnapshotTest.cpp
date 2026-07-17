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
  testMethod(testWidthSamplesUseRunBounds);
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

  bool rejectedNegativeLevel = false;
  try {
    runner::boundedFullyRefinedLeaves(-1);
  } catch (const std::invalid_argument&) {
    rejectedNegativeLevel = true;
  }
  validate(rejectedNegativeLevel);
}

void runner::tests::GridSnapshotTest::testWidthSamplesUseRunBounds() {
  validateEquals(runner::gridCellSummary(81, 10), std::string("81  (Cells 91)"));
  validateEquals(runner::gridCellSummary(1234, 5678), std::string("1,234  (Cells 6,912)"));
  validateEquals(
    runner::gridCellSummary(std::numeric_limits<std::uint64_t>::max(), 1),
    std::string("18,446,744,073,709,551,615  (Cells 18,446,744,073,709,551,615)")
  );

  const auto samples  = runner::gridSnapshotWidthSamples();
  const auto contains = [&samples](const std::string& value) {
    for (const auto& line : samples) {
      if (line.find(value) != std::string::npos) {
        return true;
      }
    }
    return false;
  };

  validate(contains(runner::Format::overheadPercentageWidthSample()));
  validate(not contains(runner::Format::memory(std::numeric_limits<unsigned long long>::max())));
  validate(not contains(runner::Format::grouped(std::numeric_limits<std::uint64_t>::max())));
  validate(contains("Cells "));
}
