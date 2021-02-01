// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MPI_TESTS_STRING_TEST_H_
#define _TARCH_MPI_TESTS_STRING_TEST_H_


#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace tarch {
  namespace mpi {
    namespace tests {
      class StringTest;
    }
  }
}


/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class tarch::mpi::tests::StringTest: public tarch::tests::TestCase {
  private:
    static tarch::logging::Log _log;

    /**
     * Tests constructors.
     */
    void testSendReceive();

  public:
    StringTest();

    /**
     * This routine is triggered by the TestCaseCollection
     */
    void run() override;
};

#endif
