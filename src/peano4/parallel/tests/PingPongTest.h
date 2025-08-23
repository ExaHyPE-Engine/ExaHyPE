// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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

    /**
     * Build up suite of test routines
     *
     * We protect the multithreaded calls with a call with a comparison of
     * tarch::mpi::Rank::getInstance().getProvidedThreadLevelSupport() to
     * MPI_THREAD_MULTIPLE. In principle, we know that Peano cannot work
     * properly without MPI's multithreaded support. However, even if the
     * MPI implementation supports multithreading, tools such as MUST
     * (http://www.itc.rwth-aachen.de/MUST) will artifically lower the
     * thread level support for correctness check and then report a
     * deadlock here (wrongly). To avoid this, we remove the unit tests, as
     * I really wanna use the thread-checkers with our unit tests.
     */
    virtual void run() override;
};


