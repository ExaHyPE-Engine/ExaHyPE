#include "Riemann.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"

#include "peano4/utils/Loop.h"
#include "peano4/datamanagement/FaceMarker.h"

void exahype2::dg::interpolateRiemannSolution(
  const peano4::datamanagement::FaceMarker&  marker,
  int                                        order,
  int                                        unknowns,
  const double* __restrict__                 InterpolationMatrix1d,
  const double* __restrict__                 coarseGridFaceQ,
  double* __restrict__                       fineGridFaceQ
) {
  exahype2::enumerator::FaceAoSLexicographicEnumerator faceEnumerator(
    marker.getSelectedFaceNumber(),
    order+1,
    1,                     // overlap
    unknowns,
    0                      // auxiliaryVariables
  );

  #if Dimensions==2
  const int totalNumberOfValues = (order+1) * 2 * unknowns;
  const int InterpolationMatrixDimensions = (order+1);   // no of rows/cols
  #else
  const int totalNumberOfValues = (order+1) * (order+1) * 2 * unknowns;
  const int InterpolationMatrixDimensions = (order+1)*(order+1);
  #endif
  std::fill_n( fineGridFaceQ, totalNumberOfValues, 0.0 );

  const int normal = marker.getSelectedFaceNumber()%Dimensions;
  dfore(fineGridDoF,order+1,normal,0) {
    dfore(coarseGridDoF,order+1,normal,0) {
      double tensorProductWeight = 1.0;
      for (int d=0; d<Dimensions; d++) {
        if (d!=normal) {
          int subMatrix = marker.getRelativePositionWithinFatherCell()(d);
          assertion(subMatrix>=0);
          assertion(subMatrix<=2);

          int col = fineGridDoF(d);
          int row = coarseGridDoF(d);
          int linearisedMatrixIndex = subMatrix * InterpolationMatrixDimensions * InterpolationMatrixDimensions
                                    + row       * InterpolationMatrixDimensions
                                    + col;
          assertion(linearisedMatrixIndex>=0);
          assertion2(linearisedMatrixIndex<3, linearisedMatrixIndex, InterpolationMatrixDimensions);
          tensorProductWeight *= InterpolationMatrix1d[linearisedMatrixIndex];
        }
      }

      tarch::la::Vector<Dimensions,int> shiftedFineGridDoF = fineGridDoF;
      tarch::la::Vector<Dimensions,int> shiftedCoarseGridDoF  = coarseGridDoF;
      for (int leftRight=0; leftRight<2; leftRight++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        shiftedFineGridDoF(normal) = leftRight;
        shiftedCoarseGridDoF(normal)  = leftRight;

        fineGridFaceQ[ faceEnumerator(shiftedFineGridDoF,unknown) ] += tensorProductWeight * coarseGridFaceQ[ faceEnumerator(shiftedCoarseGridDoF,unknown) ];
      }
    }
  }
}

void exahype2::dg::restrictAndAccumulateProjectedFacePolynomial(
  const peano4::datamanagement::FaceMarker&  marker,
  int                                        order,
  int                                        numberOfProjectedQuantities,
  int                                        unknowns,
  int                                        auxiliaryVariables,
  const double* __restrict__                 RestrictionMatrix1d,
  const double* __restrict__                 fineGridFaceQ,
  double* __restrict__                       coarseGridFaceQ
) {
  assertion(numberOfProjectedQuantities>=1);
  // @todo Remove as it should work in principle for other values as well, but I haven't tested it
  assertionEquals(numberOfProjectedQuantities,1);

  exahype2::enumerator::FaceAoSLexicographicEnumerator faceEnumerator(
    marker.getSelectedFaceNumber(),
    order+1,
    numberOfProjectedQuantities,
    unknowns,
    auxiliaryVariables
  );

  #if Dimensions==2
  [[maybe_unused]] const int totalNumberOfValues = (order+1) * (numberOfProjectedQuantities*2) * (unknowns+auxiliaryVariables);
  const int RestrictionMatrixDimensions = (order+1);   // no of rows/cols
  #else
  [[maybe_unused]] const int totalNumberOfValues = (order+1) * (order+1) * (numberOfProjectedQuantities*2) * (unknowns+auxiliaryVariables);
  const int RestrictionMatrixDimensions = (order+1)*(order+1);
  #endif

  const int normal        = marker.getSelectedFaceNumber()%Dimensions;
  const int leftRightFace = marker.getSelectedFaceNumber() < Dimensions ? 1 : 0;
  dfore(fineGridDoF,order+1,normal,0) {
    dfore(coarseGridDoF,order+1,normal,0) {
      double tensorProductWeight = 1.0;
      for (int d=0; d<Dimensions; d++) {
        if (d!=normal) {
          int subMatrix = marker.getRelativePositionWithinFatherCell()(d);
          assertion(subMatrix>=0);
          assertion(subMatrix<=2);

          int row = fineGridDoF(d);
          int col = coarseGridDoF(d);
          int linearisedMatrixIndex = subMatrix * RestrictionMatrixDimensions * RestrictionMatrixDimensions
                                    + row       * RestrictionMatrixDimensions
                                    + col;
          assertion(linearisedMatrixIndex>=0);
          assertion2(linearisedMatrixIndex<3, linearisedMatrixIndex, RestrictionMatrixDimensions);
          tensorProductWeight *= RestrictionMatrix1d[linearisedMatrixIndex];
        }
      }

      tarch::la::Vector<Dimensions,int> shiftedFineGridDoF    = fineGridDoF;
      tarch::la::Vector<Dimensions,int> shiftedCoarseGridDoF  = coarseGridDoF;
      for (int leftRight=leftRightFace * numberOfProjectedQuantities; leftRight<(leftRightFace+1)*numberOfProjectedQuantities; leftRight++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        shiftedFineGridDoF(normal)    = leftRight;
        shiftedCoarseGridDoF(normal)  = leftRight;

        assertion( faceEnumerator(shiftedCoarseGridDoF,unknown)>=0 );
        assertion( faceEnumerator(shiftedFineGridDoF,unknown)>=0 );

        coarseGridFaceQ[ faceEnumerator(shiftedCoarseGridDoF,unknown) ] += tensorProductWeight * fineGridFaceQ[ faceEnumerator(shiftedFineGridDoF,unknown) ];
      }
    }
  }
}

void exahype2::dg::clearSolutionProjection(
  int                           order,
  int                           unknowns,
  int                           auxiliaryVariables,
  int                           numberOfProjectedQuantities,
  double* __restrict__          faceQ
) {
  #if Dimensions==2
  const int numberOfDoFs = (order+1);
  #else
  const int numberOfDoFs = (order+1) * (order+1);
  #endif
  const int numberOfDoubles = numberOfDoFs * (unknowns + auxiliaryVariables) * 2 * numberOfProjectedQuantities;
  std::fill_n( faceQ, numberOfDoubles, 0.0 );
}

void exahype2::dg::clearRiemannResult(
  int                           order,
  int                           unknowns,
  double* __restrict__          faceQ
) {
  clearSolutionProjection(
    order,
    unknowns,
    0,          //    auxiliaryVariables,
    1,          //    numberOfProjectedQuantities
    faceQ
  );
}

void exahype2::dg::projectVolumetricDataOntoFaces(
  [[maybe_unused]] const double* __restrict__          cellQ,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] int                                 auxiliaryVariables,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] double* const __restrict__          faceQLeft,
  [[maybe_unused]] double* const __restrict__          faceQRight,
  [[maybe_unused]] double* const __restrict__          faceQBottom,
  [[maybe_unused]] double* const __restrict__          faceQUp
) {
#if Dimensions==2
  exahype2::enumerator::AoSLexicographicEnumerator     cellEnumerator(1,order+1,0,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator leftRightFaceEnumerator(0,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomTopFaceEnumerator(1,order+1,1,unknowns,auxiliaryVariables);

  // Project left/right
  for (int y=0; y<order+1; y++) {
    const auto left = volumeIndex(1, y);
    const auto right = volumeIndex(0, y);
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft [ leftRightFaceEnumerator(left,unknown) ] = 0.0;
    faceQRight[ leftRightFaceEnumerator(right,unknown) ] = 0.0;
  }}

  for (int y=0; y<order+1; y++) {
    const auto left = volumeIndex(1, y);
    const auto right = volumeIndex(0, y);
  for (int x=0; x<order+1; x++) {
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft [ leftRightFaceEnumerator(left,unknown) ] += BasisFunctionValuesLeft[x]       * cellQ[ cellEnumerator(0,{x,y},unknown) ];
    faceQRight[ leftRightFaceEnumerator(right,unknown) ] += BasisFunctionValuesLeft[order-x] * cellQ[ cellEnumerator(0,{x,y},unknown) ];
  }}}

  // Project up/down
  for (int x=0; x<order+1; x++) {
    const auto bottom = volumeIndex(x, 1);
    const auto up = volumeIndex(x, 0);
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator(bottom,unknown) ] = 0.0;
    faceQUp     [ bottomTopFaceEnumerator(up,unknown) ] = 0.0;
  }}

  for (int x=0; x<order+1; x++) {
    const auto bottom = volumeIndex(x, 1);
    const auto up = volumeIndex(x, 0);
  for (int y=0; y<order+1; y++) {
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator(bottom,unknown) ] += BasisFunctionValuesLeft[y]       * cellQ[ cellEnumerator(0,{x,y},unknown) ];
    faceQUp     [ bottomTopFaceEnumerator(up,unknown) ] += BasisFunctionValuesLeft[order-y] * cellQ[ cellEnumerator(0,{x,y},unknown) ];
  }}}
#endif
}

void exahype2::dg::projectVolumetricDataOntoFaces(
  [[maybe_unused]] const double* __restrict__          cellQ,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] int                                 auxiliaryVariables,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] double* const __restrict__          faceQLeft,
  [[maybe_unused]] double* const __restrict__          faceQRight,
  [[maybe_unused]] double* const __restrict__          faceQBottom,
  [[maybe_unused]] double* const __restrict__          faceQUp,
  [[maybe_unused]] double* const __restrict__          faceQFront,
  [[maybe_unused]] double* const __restrict__          faceQBack
) {
#if Dimensions==3
  exahype2::enumerator::AoSLexicographicEnumerator     cellEnumerator(1,order+1,0,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator leftRightFaceEnumerator(0,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomTopFaceEnumerator(1,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator frontBackFaceEnumerator(2,order+1,1,unknowns,auxiliaryVariables);

  // Project left/right
  for (int y=0; y<order+1; y++) {
  for (int z=0; z<order+1; z++) {
    const auto left = volumeIndex(1, y, z);
    const auto right = volumeIndex(0, y, z);
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft   [ leftRightFaceEnumerator(left,unknown) ] = 0.0;
    faceQRight  [ leftRightFaceEnumerator(right,unknown) ] = 0.0;
  }}}

  for (int y=0; y<order+1; y++) {
  for (int z=0; z<order+1; z++) {
    const auto left = volumeIndex(1, y, z);
    const auto right = volumeIndex(0, y, z);
  for (int x=0; x<order+1; x++) {
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQLeft   [ leftRightFaceEnumerator(left,unknown) ] += BasisFunctionValuesLeft[x]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQRight  [ leftRightFaceEnumerator(right,unknown) ] += BasisFunctionValuesLeft[order-x] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }}}}

  // Project down/up
  for (int x=0; x<order+1; x++) {
  for (int z=0; z<order+1; z++) {
    const auto bottom = volumeIndex(x, 1, z);
    const auto up = volumeIndex(x, 0, z);
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator(bottom,unknown) ] = 0.0;
    faceQUp     [ bottomTopFaceEnumerator(up,unknown) ] = 0.0;
  }}}

  for (int x=0; x<order+1; x++) {
  for (int z=0; z<order+1; z++) {
    const auto bottom = volumeIndex(x, 1, z);
    const auto up = volumeIndex(x, 0, z);
  for (int y=0; y<order+1; y++) {
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQBottom [ bottomTopFaceEnumerator(bottom,unknown) ] += BasisFunctionValuesLeft[y]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQUp     [ bottomTopFaceEnumerator(up,unknown) ] += BasisFunctionValuesLeft[order-y] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }}}}

  // Project front/back
  for (int x=0; x<order+1; x++) {
  for (int y=0; y<order+1; y++) {
    const auto front = volumeIndex(x, y, 1);
    const auto back = volumeIndex(x, y, 0);
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQFront [ frontBackFaceEnumerator(front,unknown) ] = 0.0;
    faceQBack  [ frontBackFaceEnumerator(back,unknown) ] = 0.0;
  }}}

  for (int x=0; x<order+1; x++) {
  for (int y=0; y<order+1; y++) {
    const auto front = volumeIndex(x, y, 1);
    const auto back = volumeIndex(x, y, 0);
  for (int z=0; z<order+1; z++) {
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    faceQFront [ frontBackFaceEnumerator(front,unknown) ] += BasisFunctionValuesLeft[z]       * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
    faceQBack  [ frontBackFaceEnumerator(back,unknown) ]  += BasisFunctionValuesLeft[order-z] * cellQ[ cellEnumerator(0,{x,y,z},unknown) ];
  }}}}
#endif
}

void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  [[maybe_unused]] const double* __restrict__          cellQ,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] int                                 auxiliaryVariables,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] double* const __restrict__          faceQLeft,
  [[maybe_unused]] double* const __restrict__          faceQRight,
  [[maybe_unused]] double* const __restrict__          faceQBottom,
  [[maybe_unused]] double* const __restrict__          faceQUp
) {
  assertionMsg(false, "not implemented yet");
}

void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  [[maybe_unused]] const double* __restrict__          cellQ,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] int                                 auxiliaryVariables,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] double* const __restrict__          faceQLeft,
  [[maybe_unused]] double* const __restrict__          faceQRight,
  [[maybe_unused]] double* const __restrict__          faceQBottom,
  [[maybe_unused]] double* const __restrict__          faceQUp,
  [[maybe_unused]] double* const __restrict__          faceQFront,
  [[maybe_unused]] double* const __restrict__          faceQBack
) {
  assertionMsg(false, "not implemented yet");
}

void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  [[maybe_unused]] const double* const __restrict__    faceQLeft,
  [[maybe_unused]] const double* const __restrict__    faceQRight,
  [[maybe_unused]] const double* const __restrict__    faceQBottom,
  [[maybe_unused]] const double* const __restrict__    faceQUp,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] const int                           auxiliaryVariables,
  [[maybe_unused]] const tarch::la::Vector<2,double>&  cellSize,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] const double* __restrict__          MassMatrixDiagonal1d,
  [[maybe_unused]] double* __restrict__                cellQ
) {
#if Dimensions==2
  [[maybe_unused]] const int strideQ = unknowns + auxiliaryVariables;

  exahype2::enumerator::AoSLexicographicEnumerator QEnumerator(
    1,           // number of cells in one memory chunk
    order+1,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables here
  );

  exahype2::enumerator::FaceAoSLexicographicEnumerator leftFaceEnumerator  (0,            order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator rightFaceEnumerator (0+Dimensions, order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomFaceEnumerator(1,            order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator upperFaceEnumerator (1+Dimensions, order+1, 1, unknowns, 0);

  dfor( dof, order+1 ) {
    const double coeffLeft    = MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[dof[0]]       * cellSize(1);
    const double coeffRight   = MassMatrixDiagonal1d[dof[1]]*BasisFunctionValuesLeft[order-dof[0]] * cellSize(1);

    const double coeffBottom  = MassMatrixDiagonal1d[dof[0]]*BasisFunctionValuesLeft[dof[1]]       * cellSize(0);
    const double coeffUp      = MassMatrixDiagonal1d[dof[0]]*BasisFunctionValuesLeft[order-dof[1]] * cellSize(0);

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
          coeffLeft   * faceQLeft   [leftFaceEnumerator(  leftDoF,  var)]
        - coeffRight  * faceQRight  [rightFaceEnumerator( rightDoF, var)]
        + coeffBottom * faceQBottom [bottomFaceEnumerator(bottomDoF,var)]
        - coeffUp     * faceQUp     [upperFaceEnumerator( upperDoF, var)];
    }
  }
#endif
}

void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  [[maybe_unused]] const double* const __restrict__    faceQLeft,
  [[maybe_unused]] const double* const __restrict__    faceQRight,
  [[maybe_unused]] const double* const __restrict__    faceQBottom,
  [[maybe_unused]] const double* const __restrict__    faceQUp,
  [[maybe_unused]] const double* const __restrict__    faceQFront,
  [[maybe_unused]] const double* const __restrict__    faceQBack,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] const int                           auxiliaryVariables,
  [[maybe_unused]] const tarch::la::Vector<3,double>&  cellSize,
  [[maybe_unused]] const double* const __restrict__    BasisFunctionValuesLeft,
  [[maybe_unused]] const double* __restrict__          MassMatrixDiagonal1d,
  [[maybe_unused]] double* __restrict__                cellQ
) {
#if Dimensions==3
  exahype2::enumerator::AoSLexicographicEnumerator QEnumerator(
    1,           // number of cells in one memory chunk
    order+1,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables here
  );

  exahype2::enumerator::FaceAoSLexicographicEnumerator leftFaceEnumerator  (0,            order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator rightFaceEnumerator (0+Dimensions, order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator bottomFaceEnumerator(1,            order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator upperFaceEnumerator( 1+Dimensions, order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator frontFaceEnumerator( 2,            order+1, 1, unknowns, 0);
  exahype2::enumerator::FaceAoSLexicographicEnumerator backFaceEnumerator(  2+Dimensions, order+1, 1, unknowns, 0);

  dfor( dof, order+1 ) {
    //coefficients of faces in x direction
    const double coeffLeft    = BasisFunctionValuesLeft[dof[0]]       * MassMatrixDiagonal1d[dof[1]]          * MassMatrixDiagonal1d[dof[2]]          * cellSize(1) * cellSize(2);
    const double coeffRight   = BasisFunctionValuesLeft[order-dof[0]] * MassMatrixDiagonal1d[dof[1]]          * MassMatrixDiagonal1d[dof[2]]          * cellSize(1) * cellSize(2);
    //coefficients of faces in y direction
    const double coeffBottom  = MassMatrixDiagonal1d[dof[0]]          * BasisFunctionValuesLeft[dof[1]]       * MassMatrixDiagonal1d[dof[2]]          * cellSize(0) * cellSize(2);
    const double coeffUp      = MassMatrixDiagonal1d[dof[0]]          * BasisFunctionValuesLeft[order-dof[1]] * MassMatrixDiagonal1d[dof[2]]          * cellSize(0) * cellSize(2);
    //coefficients of faces in z direction
    const double coeffFront   = MassMatrixDiagonal1d[dof[0]]          * MassMatrixDiagonal1d[dof[1]]          * BasisFunctionValuesLeft[dof[2]]       * cellSize(0) * cellSize(1);
    const double coeffBack    = MassMatrixDiagonal1d[dof[0]]          * MassMatrixDiagonal1d[dof[1]]          * BasisFunctionValuesLeft[order-dof[2]] * cellSize(0) * cellSize(1);

    tarch::la::Vector<Dimensions,int> leftDoF      = dof;
    tarch::la::Vector<Dimensions,int> rightDoF     = dof;
    tarch::la::Vector<Dimensions,int> frontDoF     = dof;
    tarch::la::Vector<Dimensions,int> backDoF      = dof;
    tarch::la::Vector<Dimensions,int> bottomDoF    = dof;
    tarch::la::Vector<Dimensions,int> upperDoF     = dof;

    leftDoF(0)    = 1;
    rightDoF(0)   = 0;
    bottomDoF(1)  = 1;
    upperDoF(1)   = 0;
    frontDoF(2)   = 1;
    backDoF(2)    = 0;

    for(int var=0; var<unknowns; var++) {
      cellQ[ QEnumerator(0,dof,var) ] +=
        + coeffLeft   * faceQLeft   [leftFaceEnumerator  (leftDoF,  var)]
        - coeffRight  * faceQRight  [rightFaceEnumerator (rightDoF, var)]
        + coeffBottom * faceQBottom [bottomFaceEnumerator(bottomDoF,var)]
        - coeffUp     * faceQUp     [upperFaceEnumerator (upperDoF, var)]
        + coeffFront  * faceQFront  [frontFaceEnumerator (frontDoF, var)]
        - coeffBack   * faceQBack   [backFaceEnumerator  (backDoF,  var)];
    }
  }
#endif
}
