// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParametersTest.h"

#include "runner/RuntimeParameters.h"

runner::tests::RuntimeParametersTest::RuntimeParametersTest() :
  TestCase("runner::tests::RuntimeParametersTest") {}

void runner::tests::RuntimeParametersTest::run() {
  testMethod(testNumbersParseOnlyWhenTheWholeArgumentIsANumber);
  testMethod(testTheOutputPathAlwaysEndsInASeparator);
  testMethod(testTheMeshSizeDividesOncePerLevel);
  testMethod(testDefaultsReadBackAsTheyWereGiven);
  testMethod(testBothSpellingsOfTheLoadBalancingSwitchAreConsumed);
}

void runner::tests::RuntimeParametersTest::testNumbersParseOnlyWhenTheWholeArgumentIsANumber() {
  validateEquals(runner::RuntimeParameters::parseInteger("42", "-lmax"), 42);
  validateEquals(runner::RuntimeParameters::parseInteger("-7", "-lmax"), -7);
  validateNumericalEquals(runner::RuntimeParameters::parseDouble("0.25", "-et"), 0.25);
  validateNumericalEquals(runner::RuntimeParameters::parseDouble("-1.5e2", "-et"), -150.0);

  const auto rejects = [](const auto& parse, const std::string& value) {
    try {
      parse(value, "-key");
    } catch (const std::runtime_error&) {
      return true;
    }
    return false;
  };

  validate(rejects(runner::RuntimeParameters::parseInteger, "42x"));
  validate(rejects(runner::RuntimeParameters::parseInteger, "4.2"));
  validate(rejects(runner::RuntimeParameters::parseInteger, ""));
  validate(rejects(runner::RuntimeParameters::parseInteger, "abc"));
  validate(rejects(runner::RuntimeParameters::parseDouble, "0.25m"));
  validate(rejects(runner::RuntimeParameters::parseDouble, ""));
  validate(rejects(runner::RuntimeParameters::parseDouble, "abc"));
}

void runner::tests::RuntimeParametersTest::testTheOutputPathAlwaysEndsInASeparator() {
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("solutions"), std::string("solutions/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("solutions/"), std::string("solutions/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("/tmp/a/b"), std::string("/tmp/a/b/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath(""), std::string("./"));
}

void runner::tests::RuntimeParametersTest::testTheMeshSizeDividesOncePerLevel() {
  validateNumericalEquals(runner::RuntimeParameters::divideByThreePower(1.0, 0), 1.0);
  validateNumericalEquals(runner::RuntimeParameters::divideByThreePower(1.0, 1), 1.0 / 3.0);
  validateNumericalEquals(runner::RuntimeParameters::divideByThreePower(9.0, 2), 1.0);

  bool rejectedNegativeExponent = false;
  try {
    runner::RuntimeParameters::divideByThreePower(1.0, -1);
  } catch (const std::runtime_error&) {
    rejectedNegativeExponent = true;
  }
  validate(rejectedNegativeExponent);
}

void runner::tests::RuntimeParametersTest::testDefaultsReadBackAsTheyWereGiven() {
  validateEquals(runner::RuntimeParameters::formatDefault(7), std::string("7"));
  validateEquals(runner::RuntimeParameters::parseInteger(runner::RuntimeParameters::formatDefault(7), "-key"), 7);
  validateNumericalEquals(
    runner::RuntimeParameters::parseDouble(runner::RuntimeParameters::formatDefault(0.5), "-key"),
    0.5
  );

  validateEquals(runner::RuntimeParameters::formatDefault(std::string("")), std::string("<none>"));
  validateEquals(runner::RuntimeParameters::formatDefault(std::string("out/")), std::string("out/"));
  validateEquals(
    runner::RuntimeParameters::formatTerminalTime(std::numeric_limits<double>::max()),
    std::string("unbounded")
  );
  validateEquals(runner::RuntimeParameters::formatTerminalTime(2.0), std::string("2"));
}

void runner::tests::RuntimeParametersTest::testBothSpellingsOfTheLoadBalancingSwitchAreConsumed() {
  runner::RuntimeParameters longFormParameters;
  char                      executableName[] = "ExaHyPE";
  char                      longSpelling[]   = "--disableLoadBalancing";
  std::array<char*, 2>      longForm{executableName, longSpelling};
  int                       numberOfLongFormArguments = static_cast<int>(longForm.size());
  validateEquals(longFormParameters.isLoadBalancingDisabled(), false);
  longFormParameters.applyCommandLineConfiguration(numberOfLongFormArguments, longForm.data());
  validateEquals(longFormParameters.isLoadBalancingDisabled(), true);
  validateEquals(numberOfLongFormArguments, 1);

  runner::RuntimeParameters shortFormParameters;
  char                      shortSpelling[] = "-dlb";
  std::array<char*, 2>      shortForm{executableName, shortSpelling};
  int                       numberOfShortFormArguments = static_cast<int>(shortForm.size());
  validateEquals(shortFormParameters.isLoadBalancingDisabled(), false);
  shortFormParameters.applyCommandLineConfiguration(numberOfShortFormArguments, shortForm.data());
  validateEquals(shortFormParameters.isLoadBalancingDisabled(), true);
  validateEquals(numberOfShortFormArguments, 1);
}
