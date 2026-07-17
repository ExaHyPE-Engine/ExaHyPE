// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class TextLayoutTest;
} // namespace runner::tests

class runner::tests::TextLayoutTest final : public tarch::tests::TestCase {
public:
  TextLayoutTest();
  void run() override;

private:
  void testDisplayWidthCountsCodePoints();
  void testTruncateKeepsGlyphsWhole();
  void testPadToWidthReachesTargetWidth();
  void testFitToWidthIsExact();
};
