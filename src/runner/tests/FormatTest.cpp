// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FormatTest.h"

#include "runner/Format.h"

runner::tests::FormatTest::FormatTest() :
  TestCase("runner::tests::FormatTest") {}

void runner::tests::FormatTest::run() {
  testMethod(testDurationPicksTwoMostSignificantUnits);
  testMethod(testDurationClampsAndStaysBounded);
  testMethod(testGroupedInsertsThousandSeparators);
  testMethod(testPercentageAndScientificAndRate);
  testMethod(testWallClockAndMemory);
}

void runner::tests::FormatTest::testDurationPicksTwoMostSignificantUnits() {
  // Only the two most significant non-zero units, the lower one zero-padded to two digits.
  validateEquals(runner::Format::duration(45.0), std::string("45s"));
  validateEquals(runner::Format::duration(63.0), std::string("1m 03s"));
  validateEquals(runner::Format::duration(3661.0), std::string("1h 01m"));  // seconds dropped
  validateEquals(runner::Format::duration(90000.0), std::string("1d 01h")); // 1 day + 1 hour
  validateEquals(runner::Format::duration(0.0), std::string("0s"));
}

void runner::tests::FormatTest::testDurationClampsAndStaysBounded() {
  // Negative rounds to zero; anything past the day cap collapses to ">999d" (and cannot overflow).
  validateEquals(runner::Format::duration(-5.0), std::string("0s"));
  validateEquals(runner::Format::duration(999.0 * 24.0 * 3600.0), std::string("999d 00h"));
  validateEquals(runner::Format::duration(1000.0 * 24.0 * 3600.0), std::string(">999d"));
  validateEquals(runner::Format::duration(1.0e18), std::string(">999d"));
}

void runner::tests::FormatTest::testGroupedInsertsThousandSeparators() {
  validateEquals(runner::Format::grouped(0), std::string("0"));
  validateEquals(runner::Format::grouped(42), std::string("42"));
  validateEquals(runner::Format::grouped(999), std::string("999"));
  validateEquals(runner::Format::grouped(1000), std::string("1,000"));
  validateEquals(runner::Format::grouped(1234567), std::string("1,234,567"));
}

void runner::tests::FormatTest::testPercentageAndScientificAndRate() {
  validateEquals(runner::Format::percentage(50.0), std::string("50.00 %"));
  validateEquals(runner::Format::percentage(100.0), std::string("100.00 %"));
  validateEquals(runner::Format::scientific(1234.5), std::string("1.2345e+03"));
  validateEquals(runner::Format::rate(1000.0, "cell"), std::string("1.0000e+03 cell/s"));
}

void runner::tests::FormatTest::testWallClockAndMemory() {
  validateEquals(runner::Format::wallClock(0.05), std::string("(0.05 s)"));
  validateEquals(runner::Format::wallClock(1.5), std::string("(1.50 s)"));
  // chooseMemoryUsageFormat: < 1 MiB reports KiB, < 1 GiB reports MiB.
  validateEquals(runner::Format::memory(1024ull), std::string("1.00 KiB"));
  validateEquals(runner::Format::memory(3ull * 1024ull * 1024ull), std::string("3.00 MiB"));
}
