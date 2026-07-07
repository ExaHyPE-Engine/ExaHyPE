// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TableTest.h"

#include "runner/ReportBox.h"

#include <string>

runner::tests::TableTest::TableTest() :
  TestCase("runner::tests::TableTest") {}

void runner::tests::TableTest::run() {
  testMethod(testSectionAndEntryIndent);
  testMethod(testLabelsShareOneColumnAcrossSections);
  testMethod(testEmptyTableRendersNothing);
}

void runner::tests::TableTest::testSectionAndEntryIndent() {
  runner::Table table;
  table.add("Grid", "Leaves", "6,561");
  const auto lines = table.render();

  validateEquals(lines.size(), std::size_t(2));
  // Section title sits at BlockIndent, its entry one level deeper (2 * BlockIndent).
  validateEquals(lines[0], std::string("  Grid"));
  validateEquals(lines[1], std::string("    Leaves  6,561"));
}

void runner::tests::TableTest::testLabelsShareOneColumnAcrossSections() {
  runner::Table table;
  table.add("A", "x", "1");
  table.add("B", "long", "2");
  const auto lines = table.render();

  validateEquals(lines.size(), std::size_t(4));
  validateEquals(lines[0], std::string("  A"));
  validateEquals(lines[2], std::string("  B"));
  validateEquals(lines[1].substr(0, 4), std::string("    "));
  validateEquals(lines[3].substr(0, 4), std::string("    "));
  // The label column is global, so the short and the long label align their values.
  validateEquals(lines[1].find('1'), lines[3].find('2'));
}

void runner::tests::TableTest::testEmptyTableRendersNothing() {
  runner::Table table;
  validateEquals(table.render().size(), std::size_t(0));
}
