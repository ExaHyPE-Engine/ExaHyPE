// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/tasking/Runtime.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("The initialised runtime reports its threads, its core, the unmasked threads and the calling thread") {
  GIVEN("The runtime of this process") {
    const auto& runtime = tarch::tasking::Runtime::getInstance();

    WHEN("The runtime reports its state, its threads, its core, the unmasked threads and the calling thread") {
      const bool isInitialised           = runtime.isInitialised();
      const int  numberOfThreads         = runtime.getNumberOfThreads();
      const int  coreNumber              = runtime.getCoreNumber();
      const int  numberOfUnmaskedThreads = tarch::tasking::getNumberOfUnmaskedThreads();
      const auto unmaskedThreads         = tarch::tasking::printUnmaskedThreads();
      const int  threadNumber            = runtime.getThreadNumber();

      THEN("The runtime runs one or more unmasked threads on a valid core and the calling thread is one of them") {
        VALIDATE(isInitialised);
        VALIDATE(numberOfThreads > 0);
        VALIDATE(coreNumber >= 0);
        VALIDATE(numberOfUnmaskedThreads > 0);
        VALIDATE(not unmaskedThreads.empty());
        VALIDATE(threadNumber >= 0);
        VALIDATE(threadNumber < numberOfThreads);
      }
    }
  }
}
