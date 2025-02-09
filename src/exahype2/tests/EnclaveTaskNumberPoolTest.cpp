// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveTaskNumberPoolTest.h"

#include "exahype2/EnclaveTaskNumberPool.h"

#include <algorithm>

exahype2::tests::EnclaveTaskNumberPoolTest::EnclaveTaskNumberPoolTest():
  TestCase("exahype2::tests::EnclaveTaskNumberPoolTest") {}

void exahype2::tests::EnclaveTaskNumberPoolTest::run() {
  testMethod([this]() { testAccessToPool(500, 1, 1024, 10); });
  testMethod([this]() { testAccessToPool(10, 1, 1023, 10000); });
  testMethod([this]() { testAccessToPool(10, 1, 10, 10000); });

  testMethod([this]() { testAccessToPool(500, 8, 1024, 10); });
  testMethod([this]() { testAccessToPool(10, 8, 1023, 10000); });
  testMethod([this]() { testAccessToPool(10, 8, 10, 10000); });
}

void exahype2::tests::EnclaveTaskNumberPoolTest::testAccessToPool(
  int timeSteps,
  int taskLoopIterations,
  int poolRefillSize,
  int numberOfTaskNumbersToAcquire
) {
  EnclaveTaskNumberPool pool(poolRefillSize);

  // Validate initial state
  validateEquals(pool.sizeInPool(), 0); // Available size is 0 initially
  validateEquals(pool.sizeInGenerated(), 0); // Generated size is 0 initially
  validateEquals(pool.sizeInUse(), 0); // Initially no numbers in use

  const int numberOfTotalTaskNumbersToAcquire = numberOfTaskNumbersToAcquire * taskLoopIterations;

  for (int timeStep = 0; timeStep < timeSteps; timeStep++) {
    validateEquals(pool.sizeInUse(), 0); // No numbers in use

    std::vector<int> acquiredNumbers(numberOfTotalTaskNumbersToAcquire);

    #if defined(SharedOMP)
    #pragma omp parallel
    #pragma omp master
    #pragma omp taskloop
    #endif
    for (int t = 0; t < taskLoopIterations; t++) {
      // Acquire numbers from the pool
      for (int i = 0; i < numberOfTaskNumbersToAcquire; i++) {
        const int globalIndex = t * numberOfTaskNumbersToAcquire + i;
        int number = pool.getNext();
        acquiredNumbers.at(globalIndex) = number;
      }
    }

    // Calculate expected values
    const int totalGeneratedSize = ((numberOfTotalTaskNumbersToAcquire + poolRefillSize - 1) / poolRefillSize) * poolRefillSize;
    const int leftoverNumbers = totalGeneratedSize - numberOfTotalTaskNumbersToAcquire;

    // Validate after acquiring
    validateEquals(pool.sizeInPool(), leftoverNumbers); // Leftover numbers in the pool
    validateEquals(pool.sizeInGenerated(), totalGeneratedSize); // Reflects all generated numbers
    validateEquals(pool.sizeInUse(), numberOfTotalTaskNumbersToAcquire); // All acquired numbers are "in use"

    // Find and validate the largest number
    auto maxIter = std::max_element(acquiredNumbers.begin(), acquiredNumbers.end());
    if (maxIter != acquiredNumbers.end()) {
      int largestNumber = *maxIter;
      validate(largestNumber < pool.sizeInGenerated()); // Largest acquired number should never exceed the available numbers in the pool
    }

    #if defined(SharedOMP)
    #pragma omp parallel
    #pragma omp master
    #pragma omp taskloop
    #endif
    for (int t = 0; t < taskLoopIterations; t++) {
      // Release the numbers back to the pool
      for (int i = 0; i < numberOfTaskNumbersToAcquire; i++) {
        const int globalIndex = t * numberOfTaskNumbersToAcquire + i;
        pool.release(acquiredNumbers.at(globalIndex));
      }
    }

    // Validate after releasing
    validateEquals(pool.sizeInPool(), totalGeneratedSize); // We have again all numbers available in the pool
    validateEquals(pool.sizeInUse(), 0); // No numbers in use
    validateEquals(pool.sizeInGenerated(), totalGeneratedSize); // Generated count remains unchanged
  }

  // Reset the pool and validate state
  pool.reset();
  validateEquals(pool.sizeInPool(), 0);
  validateEquals(pool.sizeInUse(), 0);
  validateEquals(pool.sizeInGenerated(), 0);
}
