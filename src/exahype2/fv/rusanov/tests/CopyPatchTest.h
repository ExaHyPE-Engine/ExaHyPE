// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::rusanov::tests {
  class CopyPatchTest;
} // namespace exahype2::fv::rusanov::tests

class exahype2::fv::rusanov::tests::CopyPatchTest: public tarch::tests::TestCase {
private:
  void testCopyPatch();

public:
  CopyPatchTest();
  virtual ~CopyPatchTest() override = default;

  virtual void run() override;
};
