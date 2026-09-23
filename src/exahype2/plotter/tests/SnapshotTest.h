// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::plotter::tests {
  class SnapshotTest;
} // namespace exahype2::plotter::tests

class exahype2::plotter::tests::SnapshotTest final : public tarch::tests::TestCase {
public:
  SnapshotTest();
  SnapshotTest(const SnapshotTest&)            = delete;
  SnapshotTest(SnapshotTest&&)                 = delete;
  SnapshotTest& operator=(const SnapshotTest&) = delete;
  SnapshotTest& operator=(SnapshotTest&&)      = delete;
  ~SnapshotTest() override                     = default;

  void run() override;

private:
  void testWritesTheFileOfOneSnapshot();
  void testCollectsEverySnapshotInTheMetaFile();
};
