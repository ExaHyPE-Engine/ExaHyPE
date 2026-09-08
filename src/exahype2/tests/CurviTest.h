// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class CurviTest;
} // namespace exahype2::tests

class exahype2::tests::CurviTest final : public tarch::tests::TestCase {
public:
  CurviTest(const CurviTest&)            = delete;
  CurviTest(CurviTest&&)                 = delete;
  CurviTest& operator=(const CurviTest&) = delete;
  CurviTest& operator=(CurviTest&&)      = delete;

  CurviTest();

  ~CurviTest() override = default;

  void run() override;

private:
  void testCreateBlock();
  void testTransverselyVaryingPerturbation();
  void testLowerFacePerturbation();
};
