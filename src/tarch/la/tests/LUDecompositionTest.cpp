// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LUDecompositionTest.h"
#include "tarch/la/LUDecomposition.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"



tarch::la::tests::LUDecompositionTest::LUDecompositionTest():
  TestCase ("tarch::la::LUDecompositionTest") {
}


void tarch::la::tests::LUDecompositionTest::run() {
  testMethod (testLUNoPivoting);
  testMethod (testLU);
  testMethod (testInversion0 );
  testMethod (testInversion1 );
}


void tarch::la::tests::LUDecompositionTest::testLUNoPivoting() {
}


void tarch::la::tests::LUDecompositionTest::testLU() {
  // Test is obviously buggy. The pivot values are doubles but the test uses
  // them as integer in line 57.
  Matrix<2,2,double> A = {
    4.0, 3.0,
    6.0, 3.0
  };
  Matrix<2,2,double> LU = {
    4.0, 3.0,
    1.5, -1.5
  };

  tarch::la::lu(A);

  validateEquals( A, LU );
}


void tarch::la::tests::LUDecompositionTest::testInversion0(){
  // checking if Lapack correctly configured. Invert
  // a diagonal matrix.

  Matrix<5,5,double> A = {
    1,0,0,0,0,
    0,2,0,0,0,
    0,0,3,0,0,
    0,0,0,4,0,
    0,0,0,0,5
  };

  Matrix<5,5,double> AInverted = {
    1,0,0,0,0,
    0,1./2,0,0,0,
    0,0,1./3,0,0,
    0,0,0,1./4,0,
    0,0,0,0,1./5
  };

  validateEquals( invert(A), AInverted );
}


void tarch::la::tests::LUDecompositionTest::testInversion1(){
  // checking if Lapack correctly configured. Invert
  // a diagonal matrix.

  Matrix<2,2,double> A = {
    -1.0, 3.0/2.0,
    1.0, -1.0
  };

  Matrix<2,2,double> AInverted = {
    2, 3,
    2, 2
  };

  validateEquals( invert(A), AInverted );
}
