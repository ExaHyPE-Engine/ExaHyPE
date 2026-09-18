// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::particles::tests {
  class TestHelpers;
} // namespace peano4::particles::tests

class peano4::particles::tests::TestHelpers final : public tarch::tests::TestCase {
public:
  TestHelpers();
  TestHelpers(const TestHelpers&)            = delete;
  TestHelpers(TestHelpers&&)                 = delete;
  TestHelpers& operator=(const TestHelpers&) = delete;
  TestHelpers& operator=(TestHelpers&&)      = delete;
  ~TestHelpers() override                    = default;

  void run() override;

private:
  void testParticleAssignedToVertexWillBeLocal();
};
