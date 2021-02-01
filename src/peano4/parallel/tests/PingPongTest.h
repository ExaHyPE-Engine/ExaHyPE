// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_PARALLEL_TESTS_PING_POINT_TEST_H_
#define _PEANO4_PARALLEL_TESTS_PING_POINT_TEST_H_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace peano4 {
  namespace parallel {
    namespace tests {
      class PingPongTest;
    }
  }
}


/**
 * This is a ping pong test for our application-specific data types
 */
class peano4::parallel::tests::PingPongTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testBuiltInType();

    /**
     * It is really important that we test - for the DaStGen-generated data
     * types - both the built-in operation and the usage with MPI_Send and
     * MPI_Recv. I had the case that the generated datatypes had a vtable,
     * i.e. an addition four bytes. These bytes are then added to the object
     * (at the front), but I had situations where they seem not to be
     * incorporated into the this pointer. In any case, programmmers should
     * not try to exchange data types with vtables, but if they do then we'll
     * see bugs when we try to exchange the generated MPI_Send with a send()
     * call on the DaStGen object.
     */
    void testDaStGenTypeIntegerMessage();
    void testDaStGenTypeStartTraversalMessage();
    void testDaStGenArray();

    void testDaStGenArrayTreeManagementMessage();

    void testMultithreadedPingPongWithBlockingReceives();
    void testMultithreadedPingPongWithBlockingSends();
    void testMultithreadedPingPongWithBlockingSendsAndReceives();

    void testMultithreadedPingPongWithNonblockingReceives();
    void testMultithreadedPingPongWithNonblockingSends();
    void testMultithreadedPingPongWithNonblockingSendsAndReceives();
  public:
    PingPongTest();
    void run() override;
};


#endif
