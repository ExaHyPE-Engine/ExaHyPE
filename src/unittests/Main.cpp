// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/Init.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

int
main(int argc, char** argv) {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> domainSize(1.0);

  tarch::init(&argc, &argv, {0.0, 0.0, false, false});
  tarch::instrumentation::Log::setInfoOutputEnabled(false);
  tarch::instrumentation::Log::setWarningOutputEnabled(false);

  // Every thread has its own floating-point environment and arms its own trap. The MPI
  // finalisation in tarch::shutdown() can raise a floating-point exception.
  std::optional<tarch::FloatingPointExceptions::ScopedTrap> testProcessTrap(std::in_place, __FILE__, __LINE__);
  peano4::init(domainOffset, domainSize);

  int returnCode = EXIT_SUCCESS;

#pragma omp parallel
  {
    const tarch::FloatingPointExceptions::ScopedTrap threadTrap(__FILE__, __LINE__);
#pragma omp barrier
#pragma omp master
    {
      {
        const tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
        returnCode = tarch::testing::runTests(argc, argv);
      }

      if (tarch::hasNonCriticalAssertionBeenViolated()) {
        LOG_ERROR("A non-critical assertion fails in the unit tests.");
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
