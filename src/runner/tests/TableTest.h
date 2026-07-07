// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner {
  namespace tests {
    class TableTest;
  }
} // namespace runner

class runner::tests::TableTest final : public tarch::tests::TestCase {
private:
  void testSectionAndEntryIndent();
  void testLabelsShareOneColumnAcrossSections();
  void testEmptyTableRendersNothing();

public:
  TableTest();
  virtual void run() override;
};
