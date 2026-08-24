// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class PatchKernelsTest;
} // namespace toolbox::blockstructured::tests

class toolbox::blockstructured::tests::PatchKernelsTest final : public tarch::tests::TestCase {
public:
  PatchKernelsTest();

  void run() override;

private:
  void testProjectPatchFillsFaces();
  void testReconstructPatchGathersInteriorAndHalo();
};
