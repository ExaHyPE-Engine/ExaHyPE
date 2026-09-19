// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class ParallelStateTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::ParallelStateTest final : public tarch::tests::TestCase {
public:
  ParallelStateTest();
  ParallelStateTest(const ParallelStateTest&)            = delete;
  ParallelStateTest(ParallelStateTest&&)                 = delete;
  ParallelStateTest& operator=(const ParallelStateTest&) = delete;
  ParallelStateTest& operator=(ParallelStateTest&&)      = delete;
  ~ParallelStateTest() override                          = default;

  void run() override;

private:
  void testStatisticsClearAndRecord();
  void testLocalParticleOutsideALocalCellBecomesVirtual();
  void testVirtualParticleInsideALocalCellBecomesLocal();
  void testDeleteVirtualParticlesKeepsTheLocalOnes();
};
