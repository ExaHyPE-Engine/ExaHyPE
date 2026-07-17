// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class TableTest;
} // namespace runner::tests

class runner::tests::TableTest final : public tarch::tests::TestCase {
public:
  TableTest();
  void run() override;

private:
  void testSectionAndEntryIndent();
  void testLabelsShareOneColumnAcrossSections();
  void testEmptyTableRendersNothing();
};
