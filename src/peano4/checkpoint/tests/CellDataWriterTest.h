// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class CellDataWriterTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::CellDataWriterTest final : public tarch::tests::TestCase {
private:
  /** append() widens float, double and long double sources into the double record buffer. */
  void testAppendWidensEverySourceType();

public:
  CellDataWriterTest();
  ~CellDataWriterTest() override = default;

  void run() override;
};
