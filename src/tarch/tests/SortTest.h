// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class SortTest;
} // namespace tarch::tests

class tarch::tests::SortTest final : public tarch::tests::TestCase {
public:
  SortTest();
  SortTest(const SortTest& other)            = delete;
  SortTest(SortTest&& other)                 = delete;
  SortTest& operator=(const SortTest& other) = delete;
  SortTest& operator=(SortTest&& other)      = delete;
  ~SortTest() override                       = default;

  void run() override;

private:
  void testSortAscendingMatchesTheStandardSort();
  void testSortAscendingKeepsEmptyAndSingleEntries();
};
