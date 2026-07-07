// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner {
  namespace tests {
    class FormatTest;
  }
} // namespace runner

class runner::tests::FormatTest final : public tarch::tests::TestCase {
private:
  void testDurationPicksTwoMostSignificantUnits();
  void testDurationClampsAndStaysBounded();
  void testGroupedInsertsThousandSeparators();
  void testPercentageAndScientificAndRate();
  void testWallClockAndMemory();

public:
  FormatTest();
  virtual void run() override;
};
