// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::checkpoint::tests {
  class ParticleDataWriterTest;
} // namespace peano4::checkpoint::tests

class peano4::checkpoint::tests::ParticleDataWriterTest final : public tarch::tests::TestCase {
private:
  /** sortParticleRows() orders the rows by the two leading number columns and keeps each row intact. */
  void testSortParticleRows();

public:
  ParticleDataWriterTest();
  ~ParticleDataWriterTest() override = default;

  void run() override;
};
