// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class GridSnapshotTest;
} // namespace runner::tests

class runner::tests::GridSnapshotTest final : public tarch::tests::TestCase {
public:
  GridSnapshotTest();
  void run() override;

private:
  void testBoundedFullyRefinedLeavesGrowsPerLevel();
  void testBoundedFullyRefinedLeavesHandlesLimits();
  void testWidthSamplesUseRunBounds();
};
