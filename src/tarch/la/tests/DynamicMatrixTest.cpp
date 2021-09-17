#include "DynamicMatrixTest.h"
#include "tarch/la/DynamicMatrix.h"


tarch::la::tests::DynamicMatrixTest::DynamicMatrixTest():
  TestCase ("tarch::la::DynamicMatrixTest") {
}


void tarch::la::tests::DynamicMatrixTest::run() {
  testMethod (testBatchedMultiplyAoS);
}


void tarch::la::tests::DynamicMatrixTest::testBatchedMultiplyAoS() {
  tarch::la::DynamicMatrix P(24,8);
  P = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0.666667, 0, 0.333333, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0.333333, 0, 0.666667, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0.666667, 0, 0.333333, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0.333333, 0, 0.666667, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0.666667, 0, 0.333333},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0.333333, 0, 0.666667},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1}
  };

  double Q[] = {
    4.0, 0.0, 0.0, 0.0, 0.0, // Q[0] as AoS
    0.1, 0.0, 0.0, 0.0, 0.0,
    4.0, 0.0, 0.0, 0.0, 0.0,
    0.1, 0.0, 0.0, 0.0, 0.0,
    4.0, 0.0, 0.0, 0.0, 0.0,
    0.1, 0.0, 0.0, 0.0, 0.0,
    4.0, 0.0, 0.0, 0.0, 0.0,
    0.1, 0.0, 0.0, 0.0, 0.0
  };

  double result[5*4*2];

  P.batchedMultiplyAoS(
    result, // image
    Q,  // preimage
    5,          // batch size, i.e. how often to apply it in one AoS rush
    4*2,
    0 // first block
  );

  validateEqualsWithParams1( result[0*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[1*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[2*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[3*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[4*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[5*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[6*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[7*5], 0.1, P.toString(true) );

  P.batchedMultiplyAoS(
    result, // image
    Q,  // preimage
    5,          // batch size, i.e. how often to apply it in one AoS rush
    4*2,
    8 // first block
  );

  validateEqualsWithParams1( result[0*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[1*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[2*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[3*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[4*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[5*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[6*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[7*5], 0.1, P.toString(true) );

  P.batchedMultiplyAoS(
    result, // image
    Q,  // preimage
    5,          // batch size, i.e. how often to apply it in one AoS rush
    4*2,
    16 // first block
  );

  validateEqualsWithParams1( result[0*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[1*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[2*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[3*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[4*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[5*5], 0.1, P.toString(true) );
  validateEqualsWithParams1( result[6*5], 0.0, P.toString(true) );
  validateEqualsWithParams1( result[7*5], 0.1, P.toString(true) );
}
