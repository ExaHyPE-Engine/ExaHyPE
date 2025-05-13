// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::utils::tests {
  class LinearisedTest;
} // namespace peano4::utils::tests

class peano4::utils::tests::LinearisedTest final: public tarch::tests::TestCase {
private:
  void testLinearisedBitset();
  void testLinearisedVectorWithoutLookup();
  void testLinearisedVectorRoundTripWithoutLookup();
  void testDelinearisedWithoutLookup();

  void testKeyComputationForLinearisedLookup();

  void testLookupTableLinearised();
  void testLookupTableDelinearised();

  void testLinearisedVector();
  void testDelinearisedVector();

public:
  LinearisedTest();
  ~LinearisedTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
