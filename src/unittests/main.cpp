// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/UnitTests.h"
#include "peano4/Init.h"
#include "peano4/UnitTests.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Init.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/UnitTests.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "toolbox/blockstructured/UnitTests.h"
#include "toolbox/loadbalancing/UnitTests.h"
#include "toolbox/particles/UnitTests.h"

int runTests() {
  int                     unitTestsErrors = 0;
  tarch::tests::TestCase* tests           = nullptr;

  tests = tarch::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = peano4::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = toolbox::blockstructured::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = toolbox::loadbalancing::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = toolbox::particles::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  tests = exahype2::getUnitTests();
  tests->run();
  unitTestsErrors += tests->getNumberOfErrors();
  delete tests;

  if (unitTestsErrors != 0) {
    logError(unitTestsErrors << " unit tests failed. Quit.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> DomainOffset = {0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> DomainSize   = {1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> DomainOffset = {0.0, 0.0, 0.0};
  const tarch::la::Vector<DIMENSIONS, double> DomainSize   = {1.0, 1.0, 1.0};
#endif

  // No MPI timeout warning or abort here.
  tarch::init(&argc, &argv, {0.0, 0.0, true});
  peano4::init(DomainOffset, DomainSize);

  int returnCode = EXIT_SUCCESS;

#if defined(PEANO_WITH_OPENMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      {
        tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
        returnCode = runTests();
      }

      if (tarch::hasNonCriticalAssertionBeenViolated()) {
        logError("Noncritical assertion has been triggered while running the unit tests. Quit.");
        returnCode = EXIT_FAILURE;
      }
#if defined(PEANO_WITH_OPENMP)
    }
  }
#endif

  peano4::shutdown();
  tarch::shutdown();
  return returnCode;
}
