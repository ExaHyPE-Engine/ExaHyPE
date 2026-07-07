// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TextLayoutTest.h"

#include "runner/TextLayout.h"

#include <cstddef>
#include <string>
#include <vector>

runner::tests::TextLayoutTest::TextLayoutTest() :
  TestCase("runner::tests::TextLayoutTest") {}

void runner::tests::TextLayoutTest::run() {
  testMethod(testDisplayWidthCountsCodePoints);
  testMethod(testTruncateKeepsGlyphsWhole);
  testMethod(testPadToWidthReachesTargetWidth);
  testMethod(testFitToWidthIsExact);
}

void runner::tests::TextLayoutTest::testDisplayWidthCountsCodePoints() {
  validateEquals(runner::displayWidth(""), std::size_t(0));
  validateEquals(runner::displayWidth("abc"), std::size_t(3));
  validateEquals(runner::displayWidth("λ"), std::size_t(1)); // 2-byte glyph, one column
  validateEquals(runner::displayWidth("λ_max"), std::size_t(5));
  validateEquals(runner::displayWidth("··"), std::size_t(2));  // 2-byte middle dots
  validateEquals(runner::displayWidth("███"), std::size_t(3)); // 3-byte block glyphs
}

void runner::tests::TextLayoutTest::testTruncateKeepsGlyphsWhole() {
  validateEquals(runner::truncateToDisplayWidth("abcdef", 3), std::string("abc"));
  validateEquals(runner::truncateToDisplayWidth("ab", 5), std::string("ab")); // shorter than the limit

  const std::string twoBlocks = runner::truncateToDisplayWidth("█████", 2);
  validateEquals(runner::displayWidth(twoBlocks), std::size_t(2));
  validateEquals(twoBlocks.size(), std::size_t(6)); // two whole 3-byte glyphs, none split
}

void runner::tests::TextLayoutTest::testPadToWidthReachesTargetWidth() {
  validateEquals(runner::displayWidth(runner::padToWidth("ab", 5)), std::size_t(5));
  validateEquals(runner::displayWidth(runner::padToWidth("λ", 4)), std::size_t(4));
  validateEquals(runner::padToWidth("abcde", 3), std::string("abcde")); // no truncation
}

void runner::tests::TextLayoutTest::testFitToWidthIsExact() {
  const std::vector<std::string> samples =
    {"", "abc", "λ_max = 1.5222e+03", "██████████", std::string(200, 'x'), "t = 1.0000e+00 · 2.0 kcell/s"};
  for (const std::string& sample : samples) {
    for (const std::size_t width : {std::size_t(1), std::size_t(10), std::size_t(40)}) {
      validateEqualsWithParams2(runner::displayWidth(runner::fitToWidth(sample, width)), width, sample, width);
    }
  }
}
