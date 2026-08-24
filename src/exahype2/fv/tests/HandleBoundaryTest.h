// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class HandleBoundaryTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::HandleBoundaryTest final : public tarch::tests::TestCase {
public:
  HandleBoundaryTest();

  void run() override;

private:
  void testCopyPreservesUniformState();
  void testEachGhostLayerUsesMatchingInteriorLayer();
  void testEachOutsideVolumeIsWrittenOnce();
};
