// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/UnitTests.h"
#include "peano4/Init.h"
#include "peano4/UnitTests.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/UnitTests.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tests/TestCase.h"

namespace {
  int runTests() {
    int numberOfErrors = 0;

    const auto runTestCollection = [&numberOfErrors](tarch::tests::TestCase* const testCollection) {
      const std::unique_ptr<tarch::tests::TestCase> ownedTestCollection{testCollection};
      ownedTestCollection->run();
      numberOfErrors += ownedTestCollection->getNumberOfErrors();
    };

    runTestCollection(tarch::getUnitTests());
    runTestCollection(peano4::getUnitTests());
    runTestCollection(exahype2::getUnitTests());

    if (numberOfErrors != 0) {
      logError("Unit tests failed: " << numberOfErrors << ".");
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }
} // namespace

int
main(int argc, char** argv) {
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0.0, 0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {1.0, 1.0, 1.0};
#endif

  tarch::init(&argc, &argv, {0.0, 0.0, false, false});
  tarch::instrumentation::Log::setInfoOutputEnabled(false);
  tarch::instrumentation::Log::setWarningOutputEnabled(false);

  // Every thread has its own floating-point environment, so every thread arms a trap. The main
  // thread resets this trap before tarch::shutdown(), so a floating-point exception inside the MPI
  // finalisation does not abort the run.
  std::optional<tarch::FloatingPointExceptions::ScopedTrap> testProcessTrap(std::in_place, __FILE__, __LINE__);
  peano4::init(domainOffset, domainSize);

  std::cout << "Rank " << tarch::mpi::Rank::getInstance().getRank() << " of "
            << tarch::mpi::Rank::getInstance().getNumberOfRanks() << " runs the unit tests with "
            << tarch::tasking::Runtime::getInstance().getNumberOfThreads() << " threads." << std::endl;

  int returnCode = EXIT_SUCCESS;

#pragma omp parallel
  {
    const tarch::FloatingPointExceptions::ScopedTrap threadTrap(__FILE__, __LINE__);
#pragma omp barrier
#pragma omp master
    {
      {
        const tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
        returnCode = runTests();
      }

      if (tarch::hasNonCriticalAssertionBeenViolated()) {
        logError("Non-critical assertion has been triggered while running the unit tests. Quit.");
        returnCode = EXIT_FAILURE;
      }
    }
#pragma omp barrier
  }

  peano4::shutdown();
  testProcessTrap.reset();
  tarch::shutdown();
  return returnCode;
}
