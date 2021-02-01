// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_PARALLEL_TESTS_NODE_TEST_H_
#define _PEANO4_PARALLEL_TESTS_NODE_TEST_H_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace peano4 {
  namespace parallel {
    namespace tests {
      class NodeTest;
    }
  }
}


/**
 * I test the mapping of vertex adjacency data onto stack numbers here.
 */
class peano4::parallel::tests::NodeTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testGetPeriodicBoundaryNumber();
    void testTagCalculation();
  public:
    NodeTest();
    void run() override;
};


#endif
