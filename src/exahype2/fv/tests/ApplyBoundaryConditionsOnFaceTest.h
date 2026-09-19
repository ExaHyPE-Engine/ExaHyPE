// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ApplyBoundaryConditionsOnFaceTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ApplyBoundaryConditionsOnFaceTest final : public tarch::tests::TestCase {
public:
  ApplyBoundaryConditionsOnFaceTest();
  ApplyBoundaryConditionsOnFaceTest(const ApplyBoundaryConditionsOnFaceTest&)            = delete;
  ApplyBoundaryConditionsOnFaceTest(ApplyBoundaryConditionsOnFaceTest&&)                 = delete;
  ApplyBoundaryConditionsOnFaceTest& operator=(const ApplyBoundaryConditionsOnFaceTest&) = delete;
  ApplyBoundaryConditionsOnFaceTest& operator=(ApplyBoundaryConditionsOnFaceTest&&)      = delete;
  ~ApplyBoundaryConditionsOnFaceTest() override                                          = default;

  void run() override;

private:
  void testCopyPreservesUniformState();
  void testEachGhostLayerUsesMatchingInteriorLayer();
  void testEachOutsideVolumeIsWrittenOnce();
};
