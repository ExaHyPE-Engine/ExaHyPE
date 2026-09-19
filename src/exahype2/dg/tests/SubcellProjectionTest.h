// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class SubcellProjectionTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::SubcellProjectionTest final : public tarch::tests::TestCase {
public:
  SubcellProjectionTest();
  SubcellProjectionTest(const SubcellProjectionTest&)            = delete;
  SubcellProjectionTest(SubcellProjectionTest&&)                 = delete;
  SubcellProjectionTest& operator=(const SubcellProjectionTest&) = delete;
  SubcellProjectionTest& operator=(SubcellProjectionTest&&)      = delete;
  ~SubcellProjectionTest() override                              = default;

  void run() override;

private:
  void testOneDimensionalIdentityLeavesDataUnchanged();
  void testTwoDimensionalIdentityLeavesDataUnchanged();
  void testThreeDimensionalIdentityLeavesDataUnchanged();
  void testProjectionPreservesAConstantField();
};
