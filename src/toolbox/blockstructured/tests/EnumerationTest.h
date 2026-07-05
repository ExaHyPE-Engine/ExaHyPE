// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class EnumerationTest;
} // namespace toolbox::blockstructured::tests

class toolbox::blockstructured::tests::EnumerationTest final : public tarch::tests::TestCase {
private:
  void testSerialiseVoxelIndexInOverlap();

public:
  EnumerationTest();
  ~EnumerationTest() override = default;

  void run() override;
};
