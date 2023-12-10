#include "RiemannTest.h"
#include "TestUtils.h"
#include "../Riemann.h"
#include "../DGUtils.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"

exahype2::dg::tests::RiemannTest::RiemannTest():
  tarch::tests::TestCase( "exahype2::dg::tests::RiemannTest" ) {
}

void exahype2::dg::tests::RiemannTest::run() {
  testMethod( testProjectVolumetricDataOntoFacesForConstantSolutionOrder3 );
  testMethod( testProjectVolumetricDataOntoFacesForConstantSolutionOrder0 );
}

void exahype2::dg::tests::RiemannTest::testProjectVolumetricDataOntoFacesForConstantSolutionOrder3() {
  const int order        = 3;
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace = nodesPerCell/nodesPerAxis;

  // 1 unknown, 1 auxiliary variable
  double cellQin[2*nodesPerCell];
  for(int i=0; i<2*nodesPerCell; i++){
    cellQin[i] = i%2==0 ? 1.0 : -1.0;
  }

  // Values on the faces: left and right, one unknown, one auxiliary variable
  double QxL[2*nodesPerFace*2];
  double QxR[2*nodesPerFace*2];
  double QyL[2*nodesPerFace*2];
  double QyR[2*nodesPerFace*2];
  double QzL[2*nodesPerFace*2];
  double QzR[2*nodesPerFace*2];

  for(int i=0; i<2*nodesPerFace*2;i++){
    QxL[i] = 0.0;
    QxR[i] = 0.0;
    QyL[i] = 0.0;
    QyR[i] = 0.0;
    QzL[i] = 0.0;
    QzR[i] = 0.0;
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
  #if Dimensions==3
  validateNumericalEquals( QxL[20],  0.0 );
  validateNumericalEquals( QxL[21],  0.0 );
  validateNumericalEquals( QxL[22],  1.0 );
  validateNumericalEquals( QxL[23], -1.0 );
  #endif

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
  validateNumericalEquals( QyL[10],  1.0 );
  validateNumericalEquals( QyL[11], -1.0 );
  validateNumericalEquals( QyL[12],  1.0 );
  validateNumericalEquals( QyL[13], -1.0 );
  #if Dimensions==3
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomTopFaceEnumerator(1,3+1,1,1,1);

  validateEquals( bottomTopFaceEnumerator({0,0,0},0), 0 );
  validateEquals( bottomTopFaceEnumerator({0,0,0},1), 1 );
  validateEquals( bottomTopFaceEnumerator({1,0,0},0), 2 );
  validateEquals( bottomTopFaceEnumerator({1,0,0},1), 3 );
  validateEquals( bottomTopFaceEnumerator({0,1,0},0), 8 );
  validateEquals( bottomTopFaceEnumerator({0,1,0},1), 9 );
  validateEquals( bottomTopFaceEnumerator({1,1,0},0), 10 );
  validateEquals( bottomTopFaceEnumerator({1,1,0},1), 11 );
  validateEquals( bottomTopFaceEnumerator({2,1,0},0), 12 );
  validateEquals( bottomTopFaceEnumerator({2,1,0},1), 13 );
  validateEquals( bottomTopFaceEnumerator({3,1,0},0), 14 );
  validateEquals( bottomTopFaceEnumerator({3,1,0},1), 15 );
  validateEquals( bottomTopFaceEnumerator({0,0,1},0), 16 );
  validateEquals( bottomTopFaceEnumerator({0,0,1},1), 17 );

  validateNumericalEquals( QyL[16],  0.0 );
  validateNumericalEquals( QyL[17],  0.0 );
  validateNumericalEquals( QyL[18],  0.0 );
  validateNumericalEquals( QyL[19],  0.0 );
  validateNumericalEquals( QyL[20],  0.0 );
  validateNumericalEquals( QyL[21],  0.0 );
  validateNumericalEquals( QyL[24],  1.0 );
  validateNumericalEquals( QyL[25], -1.0 );
  validateNumericalEquals( QyL[26],  1.0 );
  validateNumericalEquals( QyL[27], -1.0 );
  #endif

  #if Dimensions==3
  validateNumericalEquals( QzL[0],   0.0 );
  validateNumericalEquals( QzL[1],   0.0 );
  validateNumericalEquals( QzL[2],   0.0 );
  validateNumericalEquals( QzL[3],   0.0 );
  validateNumericalEquals( QzL[4],   0.0 );
  validateNumericalEquals( QzL[5],   0.0 );
  validateNumericalEquals( QzL[32],  1.0 );
  validateNumericalEquals( QzL[33], -1.0 );

  validateNumericalEquals( QzR[0],  1.0 );
  validateNumericalEquals( QzR[1], -1.0 );
  #endif
}

void exahype2::dg::tests::RiemannTest::testProjectVolumetricDataOntoFacesForConstantSolutionOrder0() {
  const int order        = 0;

  double QIn[]     = {0.1,0.2,0.3,0.4,0.5};
  double leftQ[]   = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  double rightQ[]  = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  double bottomQ[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  double topQ[]    = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  #if Dimensions==3
  double frontQ[]  = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  double backQ[]   = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  #endif

  #if Dimensions==2
  ::exahype2::dg::projectVolumetricDataOntoFaces(
    QIn,
    0,
    5,
    0,
    BasisFunctionValuesLeftP0,
    leftQ,
    rightQ,
    bottomQ,
    topQ
  );
  #endif

  #if Dimensions==3
  ::exahype2::dg::projectVolumetricDataOntoFaces(
    QIn,
    0,
    5,
    0,
    BasisFunctionValuesLeftP0,
    leftQ,
    rightQ,
    bottomQ,
    topQ,
    frontQ,
    backQ
  );
  #endif

  validateNumericalEquals( leftQ[0], 0.0 );
  validateNumericalEquals( leftQ[1], 0.0 );
  validateNumericalEquals( leftQ[2], 0.0 );
  validateNumericalEquals( leftQ[3], 0.0 );
  validateNumericalEquals( leftQ[4], 0.0 );
  validateNumericalEquals( leftQ[5], 0.1 );
}
