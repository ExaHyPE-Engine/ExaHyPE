#include "RiemannTest.h"
#include "TestUtils.h"
#include "../Riemann.h"
#include "../DGUtils.h"


exahype2::dg::tests::RiemannTest::RiemannTest():
  tarch::tests::TestCase( "exahype2::dg::tests::RiemannTest" ) {
}


void exahype2::dg::tests::RiemannTest::run() {
  testMethod( testProjectVolumetricDataOntoFacesForConstantSolution );
}


void exahype2::dg::tests::RiemannTest::testProjectVolumetricDataOntoFacesForConstantSolution() {
  const int order        = 3;
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace = nodesPerCell/nodesPerAxis;

  // 1 unknown, 1 auxiliary variable
  double cellQin[2*nodesPerCell];
  for(int i=0; i<2*nodesPerCell; i++){
    cellQin[i] = i%2==0 ? 1.0 : -1.0;
  }

  //values on the faces: left and right, one unknown, one auxiliary variable
  double QxL[2*nodesPerFace*2];
  double QxR[2*nodesPerFace*2];
  double QyL[2*nodesPerFace*2];
  double QyR[2*nodesPerFace*2];
  #if Dimensions==3
  double QzL[2*nodesPerFace*2];
  double QzR[2*nodesPerFace*2];
  #endif

  for(int i=0; i<2*nodesPerFace*2;i++){
    QxL[i] = 0.0;
    QxR[i] = 0.0;
    QyL[i] = 0.0;
    QyR[i] = 0.0;
    #if Dimensions==3
    QzL[i] = 0.0;
    QzR[i] = 0.0;
    #endif
  }

  projectVolumetricDataOntoFaces(
    cellQin,
    order,
    1,     // unknowns
    1,     // auxiliary variables
    BasisFunctionValuesLeftP3,
    QxL,
    QxR,
    QyL,
    QyR
    #if Dimensions==3
    ,QzL,
    QzR
    #endif
  );

  validateNumericalEquals( QxL[0],  0.0 );
  validateNumericalEquals( QxL[1],  0.0 );
  validateNumericalEquals( QxL[2],  1.0 );
  validateNumericalEquals( QxL[3], -1.0 );
  validateNumericalEquals( QxL[4],  0.0 );
  validateNumericalEquals( QxL[5],  0.0 );
  validateNumericalEquals( QxL[6],  1.0 );
  validateNumericalEquals( QxL[7], -1.0 );
  validateNumericalEquals( QxL[8],  0.0 );
  validateNumericalEquals( QxL[9],  0.0 );

  validateNumericalEquals( QxR[0],  1.0 );
  validateNumericalEquals( QxR[1], -1.0 );
  validateNumericalEquals( QxR[2],  0.0 );
  validateNumericalEquals( QxR[3],  0.0 );
  validateNumericalEquals( QxR[4],  1.0 );
  validateNumericalEquals( QxR[5], -1.0 );
  validateNumericalEquals( QxR[6],  0.0 );
  validateNumericalEquals( QxR[7],  0.0 );
  validateNumericalEquals( QxR[8],  1.0 );
  validateNumericalEquals( QxR[9], -1.0 );

  validateNumericalEquals( QyL[0],   0.0 );
  validateNumericalEquals( QyL[1],   0.0 );
  validateNumericalEquals( QyL[2],   0.0 );
  validateNumericalEquals( QyL[3],   0.0 );
  validateNumericalEquals( QyL[4],   0.0 );
  validateNumericalEquals( QyL[5],   0.0 );

  #if Dimensions==3
  validateNumericalEquals( QyL[(order+1)*(order+1)*2+0],   1.0 );
  validateNumericalEquals( QyL[(order+1)*(order+1)*2+1],  -1.0 );
  validateNumericalEquals( QyL[(order+1)*(order+1)*2+2],  1.0 );
  validateNumericalEquals( QyL[(order+1)*(order+1)*2+1], -1.0 );
  #else
  validateNumericalEquals( QyL[(order+1)*2+0],   1.0 );
  validateNumericalEquals( QyL[(order+1)*2+1],  -1.0 );
  validateNumericalEquals( QyL[(order+1)*2+2],  1.0 );
  validateNumericalEquals( QyL[(order+1)*2+1], -1.0 );
  #endif

  #if Dimensions==3
  validateNumericalEquals( QzL[0],  0.0 );
  validateNumericalEquals( QzL[1],  0.0 );
  validateNumericalEquals( QzL[2],  0.0 );
  validateNumericalEquals( QzL[3],  0.0 );
  validateNumericalEquals( QzL[4],  0.0 );
  validateNumericalEquals( QzL[5],  0.0 );
  validateNumericalEquals( QzL[(order+1)*(order+1)*2+0],  1.0 );
  validateNumericalEquals( QzL[(order+1)*(order+1)*2+1], -1.0 );
  #endif
}
