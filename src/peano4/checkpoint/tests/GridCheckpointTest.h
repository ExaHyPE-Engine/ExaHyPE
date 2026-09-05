// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class GridCheckpointTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::GridCheckpointTest final : public tarch::tests::TestCase {
private:
  /** createDirectoryFor() creates every directory the file path names. */
  void testCreateDirectoryFor();

  /** readExtraTable() returns the rows of the grid file's integer table and its column count. */
  void testReadExtraTable();

public:
  GridCheckpointTest();
  ~GridCheckpointTest() override = default;

  void run() override;
};
