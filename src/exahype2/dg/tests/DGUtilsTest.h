// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class DGUtilsTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::DGUtilsTest final : public tarch::tests::TestCase {
public:
  DGUtilsTest();
  DGUtilsTest(const DGUtilsTest&)            = delete;
  DGUtilsTest(DGUtilsTest&&)                 = delete;
  DGUtilsTest& operator=(const DGUtilsTest&) = delete;
  DGUtilsTest& operator=(DGUtilsTest&&)      = delete;
  ~DGUtilsTest() override                    = default;

  void run() override;

private:
  void testNodeCountsFollowTheNumberOfNodesPerAxis();
  void testGetQuadraturePointMapsTheReferenceNode();
  void testEvaluatePolynomialOrder1();
  void testEvaluatePolynomialOrder2();
  void testEvaluatePolynomialOnAFace();
};
