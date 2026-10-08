// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ConsoleTest.h"

#include "exahype2/runner/Console.h"

namespace {
  /**
   * A framed report line spans the text and two frame columns on either side.
   */
  constexpr std::size_t FrameWidth = 97;
} // namespace

exahype2::runner::tests::ConsoleTest::ConsoleTest() :
  TestCase("exahype2::runner::tests::ConsoleTest") {}

void
exahype2::runner::tests::ConsoleTest::run() {
  testMethod(testDurationPicksTwoMostSignificantUnits);
  testMethod(testDurationClampsAndStaysBounded);
  testMethod(testGroupedInsertsThousandSeparators);
  testMethod(testPercentageScientificRateAndNumber);
  testMethod(testWallClockAndMemory);
  testMethod(testDisplayWidthCountsCodePoints);
  testMethod(testTruncateKeepsGlyphsWhole);
  testMethod(testPadToWidthReachesTargetWidth);
  testMethod(testWrapBreaksAfterTheSeparator);
  testMethod(testWrapFallsBackToSpacesAndTheWidth);
  testMethod(testTableIndentsSectionsAndAlignsLabels);
  testMethod(testEmptyTableRendersNothing);
  testMethod(testReportFramesLinesAndRules);
  testMethod(testReportWrapsAWideLineInsideTheFrame);
}

void
exahype2::runner::tests::ConsoleTest::testDurationPicksTwoMostSignificantUnits() {
  validateEquals(exahype2::runner::Format::getDuration(45.0), std::string("45s"));
  validateEquals(exahype2::runner::Format::getDuration(63.0), std::string("1m 03s"));
  validateEquals(exahype2::runner::Format::getDuration(3661.0), std::string("1h 01m"));
  validateEquals(exahype2::runner::Format::getDuration(90000.0), std::string("1d 01h"));
  validateEquals(exahype2::runner::Format::getDuration(0.0), std::string("0s"));
}

void
exahype2::runner::tests::ConsoleTest::testDurationClampsAndStaysBounded() {
  validateEquals(exahype2::runner::Format::getDuration(-5.0), std::string("0s"));
  validateEquals(exahype2::runner::Format::getDuration(999.0 * 24.0 * 3600.0), std::string("999d 00h"));
  validateEquals(exahype2::runner::Format::getDuration(1000.0 * 24.0 * 3600.0), std::string(">999d"));
  validateEquals(exahype2::runner::Format::getDuration(1.0e18), std::string(">999d"));
}

void
exahype2::runner::tests::ConsoleTest::testGroupedInsertsThousandSeparators() {
  validateEquals(exahype2::runner::Format::getGroupedNumber(0), std::string("0"));
  validateEquals(exahype2::runner::Format::getGroupedNumber(42), std::string("42"));
  validateEquals(exahype2::runner::Format::getGroupedNumber(999), std::string("999"));
  validateEquals(exahype2::runner::Format::getGroupedNumber(1000), std::string("1,000"));
  validateEquals(exahype2::runner::Format::getGroupedNumber(1234567), std::string("1,234,567"));
}

void
exahype2::runner::tests::ConsoleTest::testPercentageScientificRateAndNumber() {
  validateEquals(exahype2::runner::Format::getPercentage(50.0), std::string("50.00 %"));
  validateEquals(exahype2::runner::Format::getPercentage(100.0), std::string("100.00 %"));
  validateEquals(exahype2::runner::Format::getScientific(1234.5), std::string("1.2345e+03"));
  validateEquals(exahype2::runner::Format::getRate(1000.0, "Cell"), std::string("1.0000e+03 Cell/s"));
  validateEquals(exahype2::runner::Format::getRate(2.5e6, "DoF"), std::string("2.5000e+06 DoF/s"));
  validateEquals(exahype2::runner::Format::getNumber(42), std::string("42"));
  validateEquals(exahype2::runner::Format::getNumber(2.5), std::string("2.5"));
}

void
exahype2::runner::tests::ConsoleTest::testWallClockAndMemory() {
  validateEquals(exahype2::runner::Format::getWallClock(0.05), std::string("(0.0500 s)"));
  validateEquals(exahype2::runner::Format::getWallClock(1.5), std::string("(1.5000 s)"));
  validateEquals(exahype2::runner::Format::getMemory(1024ULL), std::string("1.00 KiB"));
  validateEquals(exahype2::runner::Format::getMemory(3ULL * 1024ULL * 1024ULL), std::string("3.00 MiB"));
}

/**
 * A multi-byte glyph occupies one column.
 */
void
exahype2::runner::tests::ConsoleTest::testDisplayWidthCountsCodePoints() {
  validateEquals(exahype2::runner::getDisplayWidth(""), std::size_t(0));
  validateEquals(exahype2::runner::getDisplayWidth("abc"), std::size_t(3));
  validateEquals(exahype2::runner::getDisplayWidth("λ"), std::size_t(1));
  validateEquals(exahype2::runner::getDisplayWidth("λ_max"), std::size_t(5));
  validateEquals(exahype2::runner::getDisplayWidth("··"), std::size_t(2));
  validateEquals(exahype2::runner::getDisplayWidth("███"), std::size_t(3));
}

void
exahype2::runner::tests::ConsoleTest::testTruncateKeepsGlyphsWhole() {
  validateEquals(exahype2::runner::truncateToDisplayWidth("abcdef", 3), std::string("abc"));
  validateEquals(exahype2::runner::truncateToDisplayWidth("ab", 5), std::string("ab"));
  const std::string twoBlocks = exahype2::runner::truncateToDisplayWidth("█████", 2);
  validateEquals(exahype2::runner::getDisplayWidth(twoBlocks), std::size_t(2));
  validateEquals(twoBlocks.size(), std::size_t(6));
}

void
exahype2::runner::tests::ConsoleTest::testPadToWidthReachesTargetWidth() {
  validateEquals(exahype2::runner::getDisplayWidth(exahype2::runner::padToWidth("ab", 5)), std::size_t(5));
  validateEquals(exahype2::runner::getDisplayWidth(exahype2::runner::padToWidth("λ", 4)), std::size_t(4));
  validateEquals(exahype2::runner::padToWidth("abcde", 3), std::string("abcde"));
}

/**
 * A broken line keeps its separator. Every continuation starts at the continuation column.
 */
void
exahype2::runner::tests::ConsoleTest::testWrapBreaksAfterTheSeparator() {
  const auto lines = exahype2::runner::wrapToDisplayWidth("Label    alpha = 1 · beta = 2 · gamma = 3", 24, 9);
  validateEquals(lines.size(), std::size_t(3));
  validateEquals(lines[0], std::string("Label    alpha = 1 ·"));
  validateEquals(lines[1], std::string("         beta = 2 ·"));
  validateEquals(lines[2], std::string("         gamma = 3"));

  const auto fitting = exahype2::runner::wrapToDisplayWidth("Label    alpha = 1", 24, 9);
  validateEquals(fitting.size(), std::size_t(1));
  validateEquals(fitting[0], std::string("Label    alpha = 1"));
}

void
exahype2::runner::tests::ConsoleTest::testWrapFallsBackToSpacesAndTheWidth() {
  const auto words = exahype2::runner::wrapToDisplayWidth("aaaa bbbb cccc", 9, 2);
  validateEquals(words.size(), std::size_t(3));
  validateEquals(words[0], std::string("aaaa"));
  validateEquals(words[1], std::string("  bbbb"));
  validateEquals(words[2], std::string("  cccc"));

  const auto letters = exahype2::runner::wrapToDisplayWidth("abcdefghij", 4, 4);
  validateEquals(letters.size(), std::size_t(3));
  validateEquals(letters[0], std::string("abcd"));
  validateEquals(letters[1], std::string("efgh"));
  validateEquals(letters[2], std::string("ij"));
}

void
exahype2::runner::tests::ConsoleTest::testTableIndentsSectionsAndAlignsLabels() {
  exahype2::runner::Table table;
  table.add("Grid", "Cells", "6,561");
  const auto lines = table.render();
  validateEquals(lines.size(), std::size_t(2));
  validateEquals(lines[0], std::string("  Grid"));
  validateEquals(lines[1], std::string("    Cells  6,561"));

  exahype2::runner::Table twoSections;
  twoSections.add("A", "x", "1");
  twoSections.add("B", "long", "2");
  const auto aligned = twoSections.render();
  validateEquals(aligned.size(), std::size_t(4));
  validateEquals(aligned[0], std::string("  A"));
  validateEquals(aligned[2], std::string("  B"));
  validateEquals(aligned[1].find('1'), aligned[3].find('2'));
}

void
exahype2::runner::tests::ConsoleTest::testEmptyTableRendersNothing() {
  const exahype2::runner::Table table;
  validateEquals(table.render().size(), std::size_t(0));
}

/**
 * Report frames every line, writes one rule for repeated rules, skips empty lines and closes each
 * frame with one bottom border.
 */
void
exahype2::runner::tests::ConsoleTest::testReportFramesLinesAndRules() {
  std::vector<std::string> lines;
  exahype2::runner::Report report([&lines](const std::string& line) { lines.push_back(line); });
  report.line("Peano");
  report.rule();
  report.rule();
  report.line("");
  report.line("  Grid");
  report.close();
  report.close();
  report.line("Goodbye");

  validateEquals(lines.size(), std::size_t(7));
  for (const auto& line : lines) {
    validateEquals(exahype2::runner::getDisplayWidth(line), FrameWidth);
  }
  validate(lines[0].starts_with("┌─") and lines[0].ends_with("─┐"));
  validateEquals(lines[1], "│ " + exahype2::runner::padToWidth("Peano", FrameWidth - 4) + " │");
  validate(lines[2].starts_with("├─") and lines[2].ends_with("─┤"));
  validateEquals(lines[3], "│ " + exahype2::runner::padToWidth("  Grid", FrameWidth - 4) + " │");
  validate(lines[4].starts_with("└─") and lines[4].ends_with("─┘"));
  validateEquals(lines[5], lines[0]);
  validateEquals(lines[6], "│ " + exahype2::runner::padToWidth("Goodbye", FrameWidth - 4) + " │");
}

/**
 * A line wider than the frame continues on a framed line at the continuation column.
 */
void
exahype2::runner::tests::ConsoleTest::testReportWrapsAWideLineInsideTheFrame() {
  std::vector<std::string> lines;
  exahype2::runner::Report report([&lines](const std::string& line) { lines.push_back(line); });
  report.line(
    "    Solver  a = 1.0000e+00 · b = 1.0000e+00 · c = 1.0000e+00 · d = 1.0000e+00 · e = 1.0000e+00 · f = 1.0000e+00",
    12
  );

  validateEquals(lines.size(), std::size_t(3));
  validateEquals(
    lines[1],
    "│ "
      + exahype2::runner::padToWidth(
        "    Solver  a = 1.0000e+00 · b = 1.0000e+00 · c = 1.0000e+00 · d = 1.0000e+00 ·",
        FrameWidth - 4
      )
      + " │"
  );
  validateEquals(
    lines[2],
    "│ " + exahype2::runner::padToWidth("            e = 1.0000e+00 · f = 1.0000e+00", FrameWidth - 4) + " │"
  );
}
