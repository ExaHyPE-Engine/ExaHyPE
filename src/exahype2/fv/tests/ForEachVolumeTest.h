// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ForEachVolumeTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ForEachVolumeTest final : public tarch::tests::TestCase {
public:
  ForEachVolumeTest();
  ForEachVolumeTest(const ForEachVolumeTest&)            = delete;
  ForEachVolumeTest(ForEachVolumeTest&&)                 = delete;
  ForEachVolumeTest& operator=(const ForEachVolumeTest&) = delete;
  ForEachVolumeTest& operator=(ForEachVolumeTest&&)      = delete;
  ~ForEachVolumeTest() override                          = default;

  void run() override;

private:
  void testVisitsEveryVolumeInLinearisedOrder();
  void testWritesThroughTheValuePointer();
};
