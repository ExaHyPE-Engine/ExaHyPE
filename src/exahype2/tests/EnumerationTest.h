// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class EnumerationTest;
} // namespace exahype2::tests

class exahype2::tests::EnumerationTest final : public tarch::tests::TestCase {
public:
  EnumerationTest();
  EnumerationTest(const EnumerationTest&)            = delete;
  EnumerationTest(EnumerationTest&&)                 = delete;
  EnumerationTest& operator=(const EnumerationTest&) = delete;
  EnumerationTest& operator=(EnumerationTest&&)      = delete;
  ~EnumerationTest() override                        = default;

  void run() override;

private:
  void testSerialiseVoxelIndexInOverlap();
};
