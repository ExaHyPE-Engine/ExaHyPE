// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SortTest.h"

#include "tarch/Sort.h"

tarch::tests::SortTest::SortTest() :
  tarch::tests::TestCase("tarch::tests::SortTest") {}

void
tarch::tests::SortTest::run() {
  testMethod(testSortAscendingMatchesTheStandardSort);
  testMethod(testSortAscendingKeepsEmptyAndSingleEntries);
}

/**
 * Every prefix of a sequence with repeated, negative and signed-zero entries sorts like std::sort.
 */
void
tarch::tests::SortTest::testSortAscendingMatchesTheStandardSort() {
  const std::vector<double> sequence{3.5, -1.0, 0.0, 7.25, -1.0, 2.0, -0.0, 1.0e-300, -8.5, 3.5, 4.0, -2.0, 6.0};
  for (std::size_t size = 0; size <= sequence.size(); size++) {
    std::vector<double> sorted(sequence.begin(), sequence.begin() + static_cast<std::ptrdiff_t>(size));
    std::vector<double> expected = sorted;
    tarch::sortAscending(sorted.data(), static_cast<int>(size));
    std::sort(expected.begin(), expected.end());
    for (std::size_t i = 0; i < size; i++) {
      validateEqualsWithParams2(sorted[i], expected[i], size, i);
    }
  }

  std::vector<int> descending{9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  tarch::sortAscending(descending.data(), static_cast<int>(descending.size()));
  for (int i = 0; i < static_cast<int>(descending.size()); i++) {
    validateEqualsWithParams1(descending[static_cast<std::size_t>(i)], i, i);
  }
}

void
tarch::tests::SortTest::testSortAscendingKeepsEmptyAndSingleEntries() {
  tarch::sortAscending(static_cast<double*>(nullptr), 0);
  double single = 4.0;
  tarch::sortAscending(&single, 1);
  validateEquals(single, 4.0);
}
