// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class CellDataLoaderTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::CellDataLoaderTest final : public tarch::tests::TestCase {
private:
  /** The loader hands out the time stamp and the solver scalars of the checkpoint file. */
  void testTimeStampAndScalars();

  /** record() returns nullptr for every cell while no block is resident. */
  void testRecordWithoutResidentBlocks();

public:
  CellDataLoaderTest();
  ~CellDataLoaderTest() override = default;

  void run() override;
};
