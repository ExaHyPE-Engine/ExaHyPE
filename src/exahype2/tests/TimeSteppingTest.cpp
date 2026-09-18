// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSteppingTest.h"

#include "exahype2/TimeStepping.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr double TimeStepRatio          = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int    NumberOfThreads        = 8;
  constexpr int    NumberOfCallsPerThread = 128;

  /**
   * The time step size a cell picks. It sits between two increments of the bucket, so the
   * bucketing returns the increment below it.
   */
  constexpr double CellTimeStepSize = 1.75;
} // namespace

exahype2::tests::TimeSteppingTest::TimeSteppingTest() :
  TestCase("exahype2::tests::TimeSteppingTest") {}

void exahype2::tests::TimeSteppingTest::run() {
  testMethod(testInterpolationWeights);
  testMethod(testTruncationToGlobalTimeStepSize);
  testMethod(testBucketingOntoPowersOfTheSubdivisionFactor);
  testMethod(testConcurrentBucketingReturnsOneValue);
}

/**
 * Checks that the weight of the new face snapshot grows linearly with the cell time stamp.
 */
void exahype2::tests::TimeSteppingTest::testInterpolationWeights() {
  const std::pair<double, double> equalTimeStamps = exahype2::getInterpolationWeights(1.0, 1.0, 1.0);
  validateNumericalEquals(equalTimeStamps.first, 0.0);
  validateNumericalEquals(equalTimeStamps.second, 1.0);

  const std::pair<double, double> quarterOfTheTimeSpan = exahype2::getInterpolationWeights(1.0, 2.0, 1.25);
  validateNumericalEquals(quarterOfTheTimeSpan.first, 0.75);
  validateNumericalEquals(quarterOfTheTimeSpan.second, 0.25);

  const std::pair<double, double> newTimeStamp = exahype2::getInterpolationWeights(1.0, 2.0, 2.0);
  validateNumericalEquals(newTimeStamp.first, 0.0);
  validateNumericalEquals(newTimeStamp.second, 1.0);
}

/**
 * Checks that a cell time step size above the subdivision factor times the global one drops
 * to that product while the bucketing stays off.
 */
void exahype2::tests::TimeSteppingTest::testTruncationToGlobalTimeStepSize() {
  const double truncatedTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(10.0, 1.0, 0);
  validateNumericalEquals(truncatedTimeStepSize, TimeStepRatio);

  const double keptTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(0.5, 1.0, 0);
  validateNumericalEquals(keptTimeStepSize, 0.5);

  const double timeStepSizeWithoutGlobalBound = exahype2::discretiseAndTruncateTimeStepSizes(10.0, 0.0, -1);
  validateNumericalEquals(timeStepSizeWithoutGlobalBound, 10.0);
}

/**
 * Checks that the bucket adopts the time step size of the first call, grows by the subdivision
 * factor, and splits into equal increments.
 */
void exahype2::tests::TimeSteppingTest::testBucketingOntoPowersOfTheSubdivisionFactor() {
  const double firstTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(1.0, 0.0, 1);
  validateNumericalEquals(firstTimeStepSize, 1.0);

  const double grownTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(TimeStepRatio, 2.0, 1);
  validateNumericalEquals(grownTimeStepSize, TimeStepRatio);

  const double bucketedTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(1.0, 0.0, 1);
  validateNumericalEquals(bucketedTimeStepSize, 1.0);

  const double incrementedTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(CellTimeStepSize, 0.0, 2);
  validateNumericalEquals(incrementedTimeStepSize, 1.5);

  // The truncation caps the cell time step size before the bucketing reads it.
  const double globalTimeStepSize    = 0.5;
  const double truncatedTimeStepSize = globalTimeStepSize * TimeStepRatio;
  const double cappedTimeStepSize    = exahype2::discretiseAndTruncateTimeStepSizes(10.0, globalTimeStepSize, 1);
  validateWithParams2(
    tarch::la::smallerEquals(cappedTimeStepSize, truncatedTimeStepSize),
    cappedTimeStepSize,
    truncatedTimeStepSize
  );
  validateWithParams2(
    tarch::la::greater(cappedTimeStepSize * TimeStepRatio, truncatedTimeStepSize),
    cappedTimeStepSize,
    truncatedTimeStepSize
  );
}

/**
 * Checks that threads which read the bucket at the same time all obtain the value of a
 * serial call.
 */
void exahype2::tests::TimeSteppingTest::testConcurrentBucketingReturnsOneValue() {
  const double serialTimeStepSize = exahype2::discretiseAndTruncateTimeStepSizes(CellTimeStepSize, 0.0, 2);

  std::vector<double>      timeStepSizes(static_cast<std::size_t>(NumberOfThreads * NumberOfCallsPerThread), 0.0);
  std::vector<std::thread> threads;
  threads.reserve(NumberOfThreads);

  for (int threadIndex = 0; threadIndex < NumberOfThreads; threadIndex++) {
    threads.emplace_back([&timeStepSizes, threadIndex]() {
      const std::size_t firstEntry = static_cast<std::size_t>(threadIndex) * NumberOfCallsPerThread;

      for (int call = 0; call < NumberOfCallsPerThread; call++) {
        const std::size_t entry = firstEntry + static_cast<std::size_t>(call);

        timeStepSizes[entry] = exahype2::discretiseAndTruncateTimeStepSizes(CellTimeStepSize, 0.0, 2);
      }
    });
  }

  for (std::thread& thread : threads) {
    thread.join();
  }

  for (const double timeStepSize : timeStepSizes) {
    validateNumericalEquals(timeStepSize, serialTimeStepSize);
  }
}
