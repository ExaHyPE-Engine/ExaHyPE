#include "Riemann.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  int                                 auxiliaryVariables,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp
) {
  assertion(Dimensions==2);

  exahype2::enumerator::AoSLexicographicEnumerator     cellEnumerator(1,order+1,0,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator leftRightFaceEnumerator(0,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomTopFaceEnumerator(1,order+1,1,unknowns,auxiliaryVariables);

  // project left/right
  for (int y=0; y<order+1; y++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft [ leftRightFaceEnumerator({1,y},unknown) ] = 0.0;
    faceQRight[ leftRightFaceEnumerator({0,y},unknown) ] = 0.0;
  }

  for (int y=0; y<order+1; y++)
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft [ leftRightFaceEnumerator({1,y},unknown) ] += BasisFunctionValuesLeft[x]       * cellQ[ cellEnumerator(0,{x,y},unknown) ];
    faceQRight[ leftRightFaceEnumerator({0,y},unknown) ] += BasisFunctionValuesLeft[order-x] * cellQ[ cellEnumerator(0,{x,y},unknown) ];
  }

  // project up/down
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator({x,1},unknown) ] = 0.0;
    faceQUp     [ bottomTopFaceEnumerator({x,0},unknown) ] = 0.0;
  }

  for (int y=0; y<order+1; y++)
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator({x,1},unknown) ] += BasisFunctionValuesLeft[y]       * cellQ[ cellEnumerator(0,{x,y},unknown) ];
    faceQUp     [ bottomTopFaceEnumerator({x,0},unknown) ] += BasisFunctionValuesLeft[order-y] * cellQ[ cellEnumerator(0,{x,y},unknown) ];
  }

}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  int                                 auxiliaryVariables,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp,
  double* const __restrict__          faceQFront,
  double* const __restrict__          faceQBack
) {
  assertion(Dimensions==3);

  exahype2::enumerator::AoSLexicographicEnumerator     cellEnumerator(1,order+1,0,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator leftRightFaceEnumerator(0,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomTopFaceEnumerator(1,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator frontBackFaceEnumerator(2,order+1,1,unknowns,auxiliaryVariables);

  // project left/right
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft   [ leftRightFaceEnumerator({1,y,z},unknown) ] = 0.0;
    faceQRight  [ leftRightFaceEnumerator({0,y,z},unknown) ] = 0.0;
  }

  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft   [ leftRightFaceEnumerator({1,y,z},unknown) ] += BasisFunctionValuesLeft[x]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQRight  [ leftRightFaceEnumerator({0,y,z},unknown) ] += BasisFunctionValuesLeft[order-x] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }

  // project down/up
  for (int x=0; x<order+1; x++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator({x,1,z},unknown) ] = 0.0;
    faceQUp     [ bottomTopFaceEnumerator({x,0,z},unknown) ] = 0.0;
  }

  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator({x,1,z},unknown) ] += BasisFunctionValuesLeft[y]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQUp     [ bottomTopFaceEnumerator({x,0,z},unknown) ] += BasisFunctionValuesLeft[order-y] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }

  // project front/back
  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQFront [ frontBackFaceEnumerator({x,y,1},unknown) ] = 0.0;
    faceQBack  [ frontBackFaceEnumerator({x,y,0},unknown) ] = 0.0;
  }
  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQFront [ frontBackFaceEnumerator({x,y,1},unknown) ] += BasisFunctionValuesLeft[z]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQBack  [ frontBackFaceEnumerator({x,y,0},unknown) ] += BasisFunctionValuesLeft[order-z] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }
}


void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  int                                 auxiliaryVariables,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp
) {
  assertionMsg(false, "not implemented yet");
}


void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  int                                 auxiliaryVariables,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp,
  double* const __restrict__          faceQFront,
  double* const __restrict__          faceQBack
) {
  assertionMsg(false, "not implemented yet");
}


void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<2,double>&  cellSize,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          MassMatrixDiagonal1d,
  double* __restrict__                cellQ
){

  const int nPA = order+1; //nodes per axis
  const int strideQ = unknowns+auxiliaryVariables;

  exahype2::enumerator::AoSLexicographicEnumerator QEnumerator(
    1,           // number of cells in one memory chunk
    nPA,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables here
  );

  exahype2::enumerator::FaceAoSLexicographicEnumerator leftFaceEnumerator(0,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator rightFaceEnumerator(2,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomFaceEnumerator(1,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator upperFaceEnumerator(3,nPA,1,unknowns,0);

  dfor( dof, order+1 ) {

    const double coeffLeft    = MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[dof[0]];
    const double coeffRight   = MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[order-dof[0]];

    const double coeffBottom  = MassMatrixDiagonal1d[dof[0]]*BasisFunctionValuesLeft[dof[1]];
    const double coeffUp      = MassMatrixDiagonal1d[dof[0]]*BasisFunctionValuesLeft[order-dof[1]];

    tarch::la::Vector<Dimensions,int>    leftDoF    = dof;
    tarch::la::Vector<Dimensions,int>    rightDoF   = dof;
    tarch::la::Vector<Dimensions,int>    bottomDoF  = dof;
    tarch::la::Vector<Dimensions,int>    upperDoF   = dof;

    leftDoF(0)    = 1;
    rightDoF(0)   = 0;
    bottomDoF(1)  = 1;
    upperDoF(1)   = 0;

    for(int var=0; var<unknowns; var++) {
      cellQ[ QEnumerator(0,dof,var) ] +=
          coeffLeft   * faceQLeft   [leftFaceEnumerator(leftDoF,var)]
        - coeffRight  * faceQRight  [rightFaceEnumerator(rightDoF,var)]
        + coeffBottom * faceQBottom [bottomFaceEnumerator(bottomDoF,var)]
        - coeffUp     * faceQUp     [upperFaceEnumerator(upperDoF,var)];
    }
  }
}



void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQFront,
  const double* const __restrict__    faceQBack,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<3,double>&  cellSize,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          MassMatrixDiagonal1d,
  double* __restrict__                cellQ
  ){
  const int nPA = order+1; //nodes per axis
  const int nodesPerFace = getNodesPerCell(nPA)/nPA;

  exahype2::enumerator::AoSLexicographicEnumerator QEnumerator(
    1,           // number of cells in one memory chunk
    nPA,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables here
  );

  exahype2::enumerator::FaceAoSLexicographicEnumerator leftFaceEnumerator(0,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator rightFaceEnumerator(3,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator frontFaceEnumerator(1,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator backFaceEnumerator(4,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomFaceEnumerator(2,nPA,1,unknowns,0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator upperFaceEnumerator(5,nPA,1,unknowns,0);

  dfor( dof, order+1 ) {

    //coefficients of faces in x direction
    const double coeffLeft  = MassMatrixDiagonal1d[dof[1]]*MassMatrixDiagonal1d[dof[2]]*BasisFunctionValuesLeft[dof[0]];
    const double coeffRight = MassMatrixDiagonal1d[dof[1]]*MassMatrixDiagonal1d[dof[2]]*BasisFunctionValuesLeft[order-dof[0]];
    //coefficients of faces in y direction
    const double coeffFront = MassMatrixDiagonal1d[dof[0]]*MassMatrixDiagonal1d[dof[2]]*BasisFunctionValuesLeft[dof[1]];
    const double coeffBack  = MassMatrixDiagonal1d[dof[0]]*MassMatrixDiagonal1d[dof[2]]*BasisFunctionValuesLeft[order-dof[1]];
    //coefficients of faces in z direction
    const double coeffBottom  = MassMatrixDiagonal1d[dof[0]]*MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[dof[2]];
    const double coeffUp      = MassMatrixDiagonal1d[dof[0]]*MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[order-dof[2]];

    tarch::la::Vector<Dimensions,int> leftDoF      = dof;
    tarch::la::Vector<Dimensions,int> rightDoF     = dof;
    tarch::la::Vector<Dimensions,int> frontDoF     = dof;
    tarch::la::Vector<Dimensions,int> backDoF      = dof;
    tarch::la::Vector<Dimensions,int> bottomDoF    = dof;
    tarch::la::Vector<Dimensions,int> upperDoF     = dof;

    leftDoF(0)    = 1;
    rightDoF(0)   = 0;
    frontDoF(1)   = 1;
    backDoF(1)    = 0;
    bottomDoF(2)  = 1;
    upperDoF(2)   = 0;

    for(int var=0; var<unknowns; var++) {
      cellQ[ QEnumerator(0,dof,var) ] +=
          coeffLeft   * faceQLeft   [leftFaceEnumerator(leftDoF,var)]
        - coeffRight  * faceQRight  [rightFaceEnumerator(rightDoF,var)]
        + coeffFront  * faceQFront  [frontFaceEnumerator(frontDoF,var)]
        - coeffBack   * faceQBack   [backFaceEnumerator(frontDoF,var)]
        + coeffBottom * faceQBottom [bottomFaceEnumerator(bottomDoF,var)]
        - coeffUp     * faceQUp     [upperFaceEnumerator(upperDoF,var)];
    }
  }
}
