// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class TransferKernelsTest;
} // namespace exahype2::dg::tests

/**
 * Checks the prolongation and restriction operators the discontinuous Galerkin families use.
 */
class exahype2::dg::tests::TransferKernelsTest final : public tarch::tests::TestCase {
public:
  TransferKernelsTest();
  TransferKernelsTest(const TransferKernelsTest&)            = delete;
  TransferKernelsTest(TransferKernelsTest&&)                 = delete;
  TransferKernelsTest& operator=(const TransferKernelsTest&) = delete;
  TransferKernelsTest& operator=(TransferKernelsTest&&)      = delete;
  ~TransferKernelsTest() override                            = default;

  void run() override;

private:
  void testFaceProlongationAppliesTheOperatorOfEachSubface();
  void testFaceRestrictionSumsTheWeightedSubfaces();
  void testCellProlongationAppliesTheOperatorOfEachChild();
  void testCellRestrictionSumsTheWeightedChildren();
  void testNodalCellInterpolationReproducesTheCoarseState();
};
