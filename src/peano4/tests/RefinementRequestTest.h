// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class RefinementRequestTest;
} // namespace peano4::tests

class peano4::tests::RefinementRequestTest final : public tarch::tests::TestCase {
public:
  RefinementRequestTest();
  RefinementRequestTest(const RefinementRequestTest&)            = delete;
  RefinementRequestTest(RefinementRequestTest&&)                 = delete;
  RefinementRequestTest& operator=(const RefinementRequestTest&) = delete;
  RefinementRequestTest& operator=(RefinementRequestTest&&)      = delete;
  ~RefinementRequestTest() override                              = default;

  void run() override;

private:
  void testOperatorEquals();
  void testOperatorLess();
  void testOrderIsATotalOrder();
  void testCanonicalFormIsIndependentOfInputOrder();
  void testToString();
  void testMPI();
};
