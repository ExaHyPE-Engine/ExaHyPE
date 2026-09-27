// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class KernelsTest;
} // namespace exahype2::fv::tests

/** Checks the regrid transfer of the finite-volume loops. */
class exahype2::fv::tests::KernelsTest final : public tarch::tests::TestCase {
public:
  KernelsTest();
  KernelsTest(const KernelsTest&)            = delete;
  KernelsTest(KernelsTest&&)                 = delete;
  KernelsTest& operator=(const KernelsTest&) = delete;
  KernelsTest& operator=(KernelsTest&&)      = delete;
  ~KernelsTest() override                    = default;

  void run() override;

private:
  void testCopyKeepsEveryValue();
  void testInterpolationFromTheParentCopiesTheCoveringVolume();
  void testRestrictionFromTheChildrenAveragesTheCoveredVolumes();
};
