// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::amr::tests {
  class InterpolationTest;
} // namespace exahype2::amr::tests

class exahype2::amr::tests::InterpolationTest final : public tarch::tests::TestCase {
public:
  InterpolationTest();
  ~InterpolationTest() override = default;

  void run() override;

private:
  void testInterpolateHaloLayer();
  void testInterpolateHaloLayerFromCoarseCell();
  void testInterpolateCell();
  void testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderReflection();
  void testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderTangentialSwap();
  void testInterpolateHaloLayerWithTangentialSlopePreservesFreeStream();
  void testInterpolateHaloLayerWithTangentialSlopeIsLinearExact();
};
