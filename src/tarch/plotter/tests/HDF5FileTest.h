// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class HDF5FileTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::HDF5FileTest final : public tarch::tests::TestCase {
private:
  /**
   * Every rank writes its own row range of a two-column Float64 dataset and a
   * one-dimensional Int64 dataset; the odd ranks contribute zero rows to a
   * further dataset, and two tables are filled from the table-writing rank.
   */
  void testDatasetsAndTables();

  /**
   * Every rank passes holdsData == false, so the global master alone creates
   * the file and fills a table.
   */
  void testWithoutDataHoldingRanks();

  /**
   * Only the global master holds data, so every other rank receives nullptr
   * and the master's rows form the whole dataset.
   */
  void testSubsetOfDataHoldingRanks();

public:
  HDF5FileTest();
  ~HDF5FileTest() override = default;

  void run() override;
};
