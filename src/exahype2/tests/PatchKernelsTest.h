// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class PatchKernelsTest;
} // namespace exahype2::tests

class exahype2::tests::PatchKernelsTest final : public tarch::tests::TestCase {
public:
  PatchKernelsTest();

  void run() override;

private:
  void testProjectPatchFillsFaces();
  void testReconstructPatchGathersInteriorAndHalo();
};
