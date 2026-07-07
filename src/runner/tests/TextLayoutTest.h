// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner {
  namespace tests {
    class TextLayoutTest;
  }
} // namespace runner

class runner::tests::TextLayoutTest final : public tarch::tests::TestCase {
private:
  void testDisplayWidthCountsCodePoints();
  void testTruncateKeepsGlyphsWhole();
  void testPadToWidthReachesTargetWidth();
  void testFitToWidthIsExact();

public:
  TextLayoutTest();
  virtual void run() override;
};
