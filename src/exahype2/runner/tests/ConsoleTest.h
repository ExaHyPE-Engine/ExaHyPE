// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::runner::tests {
  class ConsoleTest;
} // namespace exahype2::runner::tests

class exahype2::runner::tests::ConsoleTest final : public tarch::tests::TestCase {
public:
  ConsoleTest();
  ConsoleTest(const ConsoleTest&)            = delete;
  ConsoleTest(ConsoleTest&&)                 = delete;
  ConsoleTest& operator=(const ConsoleTest&) = delete;
  ConsoleTest& operator=(ConsoleTest&&)      = delete;
  ~ConsoleTest() override                    = default;

  void run() override;

private:
  void testDurationPicksTwoMostSignificantUnits();
  void testDurationClampsAndStaysBounded();
  void testGroupedInsertsThousandSeparators();
  void testPercentageScientificRateAndNumber();
  void testWallClockAndMemory();
  void testDisplayWidthCountsCodePoints();
  void testTruncateKeepsGlyphsWhole();
  void testPadToWidthReachesTargetWidth();
  void testWrapBreaksAfterTheSeparator();
  void testWrapFallsBackToSpacesAndTheWidth();
  void testTableIndentsSectionsAndAlignsLabels();
  void testEmptyTableRendersNothing();
  void testReportFramesLinesAndRules();
  void testReportWrapsAWideLineInsideTheFrame();
};
