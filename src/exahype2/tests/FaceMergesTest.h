// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class FaceMergesTest;
} // namespace exahype2::tests

class exahype2::tests::FaceMergesTest final : public tarch::tests::TestCase {
public:
  FaceMergesTest();
  FaceMergesTest(const FaceMergesTest&)            = delete;
  FaceMergesTest(FaceMergesTest&&)                 = delete;
  FaceMergesTest& operator=(const FaceMergesTest&) = delete;
  FaceMergesTest& operator=(FaceMergesTest&&)      = delete;
  ~FaceMergesTest() override                       = default;

  void run() override;

private:
  void testOverlappingVolumesCopyTheNeighbourLayers();
  void testProjectionHalvesCopyTheNeighbourHalf();
};
