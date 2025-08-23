// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace peano4 {
  namespace stacks {
    namespace tests {
      class STDVectorStackOverObjectsWithSmartPointersTest;
    }
  }
}


/**
 * This is a ping pong test for our application-specific data types
 */
class peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testMPIExchangeOfSingleObject();
  public:
    STDVectorStackOverObjectsWithSmartPointersTest();
    virtual void run() override;
};


