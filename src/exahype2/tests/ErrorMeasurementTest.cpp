// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurementTest.h"

#include "exahype2/ErrorMeasurement.h"
#include "tarch/mpi/Rank.h"

namespace {
  /**
   * Stands in for a generated solver: The first unknown grows with the time stamp and the time
   * step size, and the second one takes the first coordinate of the degree of freedom.
   */
  struct LinearSolver {
    static void analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const double* const                          state,
      double* const                                solution
    ) {
      solution[0] = state[0] + timeStamp + timeStepSize;
      solution[1] = x(0);
    }
  };
} // namespace

exahype2::tests::ErrorMeasurementTest::ErrorMeasurementTest() :
  tarch::tests::TestCase("exahype2::tests::ErrorMeasurementTest") {}

void exahype2::tests::ErrorMeasurementTest::run() {
  testMethod(testReductionSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks);
  testMethod(testAccumulationWeightsTheMeasuredUnknownsOnly);
  testMethod(testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom);
  testMethod(testRecordedSolutionErrorReachesTheMeasurement);
}

/**
 * Checks that the solver reads the auxiliary variable of the degree of freedom and that the
 * weighted error against its analytical solution reaches the caller's accumulator.
 */
void exahype2::tests::ErrorMeasurementTest::
  testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom() {
  constexpr int MeasuredUnknowns[] = {0, 1};
  const double  value[]            = {1.0, 2.0, 99.0};
  double        errors[3]{};

  tarch::la::Vector<DIMENSIONS, double> x(0.0);
  x(0) = 4.0;

  exahype2::accumulateSolutionError<LinearSolver, 2, 1>(MeasuredUnknowns, value, x, 0.4, 0.1, 2.0, errors);

  validateNumericalEquals(errors[0], 2.0 * 0.5 + 2.0 * 2.0);
  validateNumericalEquals(errors[1], 2.0 * 0.25 + 2.0 * 4.0);
  validateNumericalEquals(errors[2], 2.0);
}

/**
 * Checks that the recording variant adds the error of one degree of freedom to the measurement.
 */
void exahype2::tests::ErrorMeasurementTest::testRecordedSolutionErrorReachesTheMeasurement() {
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  constexpr int MeasuredUnknowns[] = {0};
  const double  value[]            = {1.0, 2.0, 99.0};

  tarch::la::Vector<DIMENSIONS, double> x(0.0);

  exahype2::ErrorMeasurement measurement;
  measurement.open((tarch::tests::getWritableTestDirectory() / "RecordedSolutionErrorTest").string(), 0.0);

  exahype2::accumulateAndRecordSolutionError<LinearSolver, 2, 1>(
    MeasuredUnknowns,
    value,
    x,
    0.9,
    0.1,
    1.0,
    measurement
  );

  const auto norms = measurement.reduceAndRecord(0.0);
  validateNumericalEquals(norms[0], numberOfRanks);
  validateNumericalEquals(norms[1], std::sqrt(numberOfRanks));
  validateNumericalEquals(norms[2], 1.0);
}

/**
 * Checks that the accumulation weights the listed unknowns and skips every other one.
 */
void exahype2::tests::ErrorMeasurementTest::testAccumulationWeightsTheMeasuredUnknownsOnly() {
  constexpr int MeasuredUnknowns[] = {0, 2};
  const float   value[]            = {1.0F, 100.0F, 4.0F};
  const double  exactSolution[]    = {3.0, 0.0, 3.0};
  double        errors[3]          = {0.5, 0.25, 1.5};

  exahype2::accumulateError(MeasuredUnknowns, value, exactSolution, 0.5, errors);

  validateNumericalEquals(errors[0], 0.5 + 0.5 * 2.0 + 0.5 * 1.0);
  validateNumericalEquals(errors[1], 0.25 + 0.5 * 4.0 + 0.5 * 1.0);
  validateNumericalEquals(errors[2], 2.0);
}

/**
 * Checks that the reduction sums the L1 and L2 norms over all ranks, takes the maximum for the
 * Linf norm and clears the accumulators.
 */
void exahype2::tests::ErrorMeasurementTest::testReductionSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks() {
  const auto rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  exahype2::ErrorMeasurement measurement;
  measurement.open((tarch::tests::getWritableTestDirectory() / "ErrorMeasurementTest").string(), 0.0);
  const double firstCell[3]  = {1.0, 4.0, rank};
  const double secondCell[3] = {2.0, 5.0, 0.5};
  measurement.add(firstCell);
  measurement.add(secondCell);

  const auto norms = measurement.reduceAndRecord(0.25);
  validateNumericalEquals(norms[0], 3.0 * numberOfRanks);
  validateNumericalEquals(norms[1], std::sqrt(9.0 * numberOfRanks));
  validateNumericalEquals(norms[2], std::max(numberOfRanks - 1.0, 0.5));

  const auto cleared = measurement.reduceAndRecord(0.5);
  validateNumericalEquals(cleared[0], 0.0);
  validateNumericalEquals(cleared[1], 0.0);
  validateNumericalEquals(cleared[2], 0.0);
}
