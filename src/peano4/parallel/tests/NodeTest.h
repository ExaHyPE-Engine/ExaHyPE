// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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

    /**
     * Boundary data exchange test case
     *
     *
     * ## First step: top right corner
     *
     * Pass in the vertex at the top right corner of a unit cube/square
     * and set its adjacency information such as if we had periodic BCs
     * in all directions. So we should exchange along all coordinate
     * axes and along the diagonals, too. This means that we pipe out
     * data according to the table below. We also validate that the
     * corresponding input stacks are different.
     *
     * | direction          | to        | from
     * | ------------------ | --------- | ---------
     * | top                | 8         | 16
     * | diagonal top right | 7         | 15
     * | right              | 10        | 18
     *
     *
     * ## Second step: bottom left corner
     *
     * Counter part on bottom left vertex of domain with an adjacency list
     * of [-2,-2,-2,0]. This gives:
     *
     * | direction            | to        | from
     * | -------------------- | --------- | ---------
     * | left                 | 11        | 19
     * | diagonal left bottom | 14        | 22
     * | bottom               | 13        | 21
     *
     *
     * ## Third step:
     *
     * We have to find out that the stacks are properly matching once we mirror
     * (copy) them onto each other. According to the tables above we'd expect
     * the following outcome:
     *
     * | in    | out
     * | ----- | -----
     * | 7     | 22
     * | 8     | 21
     * | 10    | 19
     * | 11    | 18
     * | 13    | 16
     * | 14    | 15
     *
     * We see immediately from the numbering, that this is "just" kind of the
     * inverse index.
     */
    void testGetOutputStacksForPeriodicBoundaryExchange();

    void testTagCalculation();
  public:
    NodeTest();
    virtual void run() override;
};


