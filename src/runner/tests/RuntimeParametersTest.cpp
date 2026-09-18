// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParametersTest.h"

#include "peano4/sfc/ActiveCurve.h"
#include "runner/RuntimeParameters.h"

runner::tests::RuntimeParametersTest::RuntimeParametersTest() :
  tarch::tests::TestCase("runner::tests::RuntimeParametersTest") {}

void runner::tests::RuntimeParametersTest::run() {
  testMethod(testNumbersParseFromTheWholeArgument);
  testMethod(testTheOutputPathAlwaysEndsInASeparator);
  testMethod(testTheMeshSizeDividesOncePerLevel);
  testMethod(testDefaultsReadBackAsTheyWereGiven);
  testMethod(testBothSpellingsOfTheLoadBalancingSwitchAreConsumed);
  testMethod(testTheMaximumTerminalTimeBoundsTheMinimumOne);
}

void runner::tests::RuntimeParametersTest::testNumbersParseFromTheWholeArgument() {
  validateEquals(runner::RuntimeParameters::parseInteger("42", "-lmax"), 42);
  validateEquals(runner::RuntimeParameters::parseInteger("-7", "-lmax"), -7);
  validateNumericalEquals(runner::RuntimeParameters::parseDouble("0.25", "-et"), 0.25);
  validateNumericalEquals(runner::RuntimeParameters::parseDouble("-1.5e2", "-et"), -150.0);
}

void runner::tests::RuntimeParametersTest::testTheOutputPathAlwaysEndsInASeparator() {
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("solutions"), std::string("solutions/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("solutions/"), std::string("solutions/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath("/tmp/a/b"), std::string("/tmp/a/b/"));
  validateEquals(runner::RuntimeParameters::normaliseOutputPath(""), std::string("./"));
}

void runner::tests::RuntimeParametersTest::testTheMeshSizeDividesOncePerLevel() {
  constexpr double SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  validateNumericalEquals(runner::RuntimeParameters::divideBySubdivisionFactorPower(1.0, 0), 1.0);
  validateNumericalEquals(runner::RuntimeParameters::divideBySubdivisionFactorPower(1.0, 1), 1.0 / SubdivisionFactor);
  validateNumericalEquals(
    runner::RuntimeParameters::divideBySubdivisionFactorPower(SubdivisionFactor * SubdivisionFactor, 2),
    1.0
  );
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

/** A maximum terminal time from the command line or the environment lowers a larger minimum one. */
void runner::tests::RuntimeParametersTest::testTheMaximumTerminalTimeBoundsTheMinimumOne() {
  runner::RuntimeParameters commandLineParameters;
  commandLineParameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");
  char                 executableName[] = "ExaHyPE";
  char                 option[]         = "-tmax";
  char                 bound[]          = "1.0";
  std::array<char*, 3> arguments{executableName, option, bound};
  int                  numberOfArguments = static_cast<int>(arguments.size());
  commandLineParameters.applyCommandLineConfiguration(numberOfArguments, arguments.data());
  commandLineParameters.boundTerminalTimes();
  validateNumericalEquals(commandLineParameters.getMinTerminalTime(), 1.0);
  validateNumericalEquals(commandLineParameters.getMaxTerminalTime(), 1.0);

  runner::RuntimeParameters environmentParameters;
  environmentParameters.setGeneratedDefaults(10.0, 10.0, -1, -1, "");
  setenv("PEANO_MAX_TERMINAL_TIME", "0.5", 1); // NOLINT(concurrency-mt-unsafe)
  environmentParameters.boundTerminalTimes();
  validateNumericalEquals(environmentParameters.getMinTerminalTime(), 0.5);
  validateNumericalEquals(environmentParameters.getMaxTerminalTime(), 0.5);

  runner::RuntimeParameters shorterRunParameters;
  shorterRunParameters.setGeneratedDefaults(0.1, 0.1, -1, -1, "");
  shorterRunParameters.boundTerminalTimes();
  unsetenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
  validateNumericalEquals(shorterRunParameters.getMinTerminalTime(), 0.1);
  validateNumericalEquals(shorterRunParameters.getMaxTerminalTime(), 0.1);
}
