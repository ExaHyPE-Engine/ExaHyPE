// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class InterpolationTest;
} // namespace toolbox::blockstructured::tests

class toolbox::blockstructured::tests::InterpolationTest final : public tarch::tests::TestCase {
private:
  void testInterpolateHaloLayer();
  void testInterpolateHaloLayerFromCoarseCell();
  void testInterpolateCell();
  void testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderReflection();
  void testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderTangentialSwap();
  void testInterpolateHaloLayerWithTangentialSlopePreservesFreeStream();
  void testInterpolateHaloLayerWithTangentialSlopeIsLinearExact();

public:
  InterpolationTest();
  ~InterpolationTest() override = default;

  void run() override;
};
