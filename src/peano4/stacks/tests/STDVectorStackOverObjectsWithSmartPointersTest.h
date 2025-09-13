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
 * This is a ping pong test for objects hosting some smart pointers and their stack
 *
 * This routine requires a couple of generated/C++ files that you might want
 * to regenerate sometimes. To do so, switch into the test directory and type
 * in
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * export PYTHONPATH=~/git/Peano/python
 * python3 prepare-test-datastructures.py
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *
 *
 */
class peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    /**
     * The objects with the smart pointers rely on flattening internally. We
     * check if this flatteneing works properly.
     */
    void testFlattening();

    void testMPIExchangeOfFlattenedObject();

    /**
     * Works only for more than one MPI rank and then checks only the flattened
     * object, as we do not support sends or receives on objects with smart
     * pointers directly. We can use the original object directly, as long as we
     * use the generated send() and receive(). These guys do the flatteneing
     * inside the routine.
     */
    void testMPIExchangeOfSingleObject();

    /**
     * Test if stack implementation works
     */
    void testStack(int repetitions);

    void testSingleStackExchange();
    void testMultipleConcurrentStackExchanges();
  public:
    STDVectorStackOverObjectsWithSmartPointersTest();
    virtual void run() override;
};


