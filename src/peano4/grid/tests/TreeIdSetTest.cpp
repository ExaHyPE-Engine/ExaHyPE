// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeIdSetTest.h"

#include "peano4/grid/TreeIdSet.h"
#include "tarch/tests/TestMacros.h"

peano4::grid::tests::TreeIdSetTest::TreeIdSetTest():
  TestCase("peano4::grid::tests::TreeIdSetTest") {}

void peano4::grid::tests::TreeIdSetTest::run() {
  testMethod(testInsertKeepsSetSemantics);
  testMethod(testScalesToLargeNumbersOfTreeIds);
}

void peano4::grid::tests::TreeIdSetTest::testInsertKeepsSetSemantics() {
  peano4::grid::TreeIdSet ids{3, 1, 3};

  validateEquals(ids.size(), static_cast<std::size_t>(2));
  validateEquals(ids.count(1), static_cast<std::size_t>(1));
  validateEquals(ids.count(3), static_cast<std::size_t>(1));
  validateEquals(ids.count(7), static_cast<std::size_t>(0));

  ids.insert(7);
  ids.insert(7);
  validateEquals(ids.size(), static_cast<std::size_t>(3));
  validateEquals(ids.count(7), static_cast<std::size_t>(1));

  ids.erase(3);
  validateEquals(ids.size(), static_cast<std::size_t>(2));
  validateEquals(ids.count(3), static_cast<std::size_t>(0));

  ids.clear();
  validate(ids.empty());
}

void peano4::grid::tests::TreeIdSetTest::testScalesToLargeNumbersOfTreeIds() {
  peano4::grid::TreeIdSet ids;
  constexpr int           NumberOfIds = 4096;

  for (int i = 1; i <= NumberOfIds; ++i) {
    ids.insert(i);
  }

  validateEquals(ids.size(), static_cast<std::size_t>(NumberOfIds));
  for (int i = 1; i <= NumberOfIds; ++i) {
    validateWithParams1(ids.count(i) == 1, i);
  }

  ids.erase(NumberOfIds / 2);
  validateEquals(ids.size(), static_cast<std::size_t>(NumberOfIds - 1));
  validateEquals(ids.count(NumberOfIds / 2), static_cast<std::size_t>(0));
}
