// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_MULTIPRECISION_TESTS_COMPRESSED_FLOATING_POINT_NUMBERS_TESTS_
#define _TOOLBOX_MULTIPRECISION_TESTS_COMPRESSED_FLOATING_POINT_NUMBERS_TESTS_
 

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace toolbox {
  namespace multiprecision {
    namespace tests {
      class CompressedFloatingPointNumbersTest;
    }
  }
}
 

/**
 * This is just a default test case that demonstrated how to write unit tests 
 * in Peano. Feel free to rename, remove, or duplicate it. 
 */ 
class toolbox::multiprecision::tests::CompressedFloatingPointNumbersTest: public tarch::tests::TestCase {
  private:
    static tarch::logging::Log  _log;

    void testDecomposeWithLongInt();
    void testDecomposeWithInt();
    void testDecomposeWithChar();
    void testBatchDecomposition();
    void testBatchDecompositionWithArrayOfFour();
    void testErrorComputation();

    void testTinyValues();

    /**
     * Something taken from the ParCo presentation
     */
    void testDecompose();

    /**
     * From ExaHyPE:
     *
  assertion in file /home/tobias/git/ExaHyPE/Code/./ExaHyPE/exahype/solvers/ADERDGSolver.cpp, line 2983 failed: tarch::la::equals( DataHeap::getInstance().getData(heapIndex)[i], reconstructedValue, CompressionAccuracy )
  parameter DataHeap::getInstance().getData(heapIndex)[i]: 1.68855695855260723448e+00
  parameter reconstructedValue: 1.01768855695855017984e+03
  parameter DataHeap::getInstance().getData(heapIndex)[i] - reconstructedValue: -1.01599999999999761258e+03
  parameter CompressionAccuracy: 1.00000000000000006228e-09
  parameter bytesForMantissa: 5
  ExaHyPE-Euler: /home/tobias/git/ExaHyPE/Code/./ExaHyPE/exahype/solvers/ADERDGSolver.cpp:2983: void exahype::solvers::ADERDGSolver::glueTogether(int, int, int): Assertion `false' failed.

     But I used /4.0 here for the accuracy.


     */
    void testComposeDecompose();
public:
    CompressedFloatingPointNumbersTest();
    virtual ~CompressedFloatingPointNumbersTest();
     
    virtual void run();
};


#endif
