#include "Interpolation.h"
#include "Enumeration.h"

#include "peano4/utils/Loop.h"
#include "tarch/la/DynamicMatrix.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"

namespace {
  tarch::logging::Log _log( "toolbox::blockstructured" );

  tarch::multicore::BooleanSemaphore  interpolationMapSemaphore;
}


tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch) {
  #if Dimensions==2
  const int patchSize       = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
  const int numberOfPatches = 3*3;
  #else
  const int patchSize       = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
  const int numberOfPatches = 3*3*3;
  #endif

  tarch::la::DynamicMatrix* result = new tarch::la::DynamicMatrix(patchSize*numberOfPatches,patchSize);

  int currentRow = 0;
  dfor3(patchIndex)
    dfor(volumeIndex,numberOfDoFsPerAxisInPatch) {
      tarch::la::Vector<Dimensions,int> sourceCell = (volumeIndex + patchIndex*numberOfDoFsPerAxisInPatch) / 3;
      int sourceCellLinearised = peano4::utils::dLinearised(sourceCell,numberOfDoFsPerAxisInPatch);
      (*result)(currentRow,sourceCellLinearised) = 1.0;
      currentRow++;
    }
  enddforx

  return result;
}


tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch, int normal, int overlap) {
  #if Dimensions==2
  const int facePatchSize   = numberOfDoFsPerAxisInPatch * 2 * overlap;
  const int numberOfPatches = 3;
  #else
  const int facePatchSize   = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2 * overlap;
  const int numberOfPatches = 3*3;
  #endif

  tarch::la::DynamicMatrix* result = new tarch::la::DynamicMatrix(facePatchSize*numberOfPatches,facePatchSize);

  dfore(patchIndex,3,normal,0) {
    dfore(volumeIndex,numberOfDoFsPerAxisInPatch,normal,0) {
      for (int i=0; i<overlap; i++) {
        tarch::la::Vector<Dimensions,int> coarseVolume = patchIndex * numberOfDoFsPerAxisInPatch + volumeIndex;
        coarseVolume(normal) = i;
        coarseVolume /= 3;
        tarch::la::Vector<Dimensions,int> fineVolume = volumeIndex;
        fineVolume(normal) = i;

        int row = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        ) +
        serialisePatchIndexInOverlap(
          patchIndex,
          3,
          1,
          normal
        ) * facePatchSize;
        int col = serialisePatchIndexInOverlap(
          coarseVolume,
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        (*result)(row,col) = 1.0;
      }
    }
  }

  return result;
}



tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch, int normal) {
  logTraceInWith2Arguments( "createLinearInterpolationMatrix(...)", numberOfDoFsPerAxisInPatch, normal );
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  /*
   * See comments on missing diagonal element
   * ========================================
    P1d(0,0) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(0,1) = -1.0/3.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) = -1.0/3.0;
  */

  P1d(0,0) =  1.0;
  P1d(0,1) =  0.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;

  logDebug( "createLinearInterpolationMatrix(...)", "1d matrix: " << P1d.toString() );

  int pattern = 0;
  switch (normal) {
    case 0:
      pattern = 1;
      break;
    case 1:
      pattern = numberOfDoFsPerAxisInPatch;
      break;
    case 2:
      pattern = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
      break;
  }

  #if Dimensions==3
    tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d, P1d, false);
    P->insertEmptyColumns(pattern,pattern,pattern);
    P->replicateRows( pattern, 2, pattern, false );
    logTraceOutWith2Arguments( "createLinearInterpolationMatrix(...)", P1d.toString(), P->toString() );
    return P;
  #else
    tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d );
    P->insertEmptyColumns(pattern,pattern,pattern);
    logDebug( "createLinearInterpolationMatrix(...)", "matrix with extended columns: " << P->toString() );
    P->replicateRows( pattern, 2, pattern, false );
    logTraceOutWith1Argument( "createLinearInterpolationMatrix(...)", P->toString() );
    return P;
  #endif
}


tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch) {
  logTraceInWith1Argument( "createLinearInterpolationMatrix(...)", numberOfDoFsPerAxisInPatch );
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  P1d(0,0) =  1.0;
  P1d(0,1) =  0.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;

  logDebug( "createLinearInterpolationMatrix(...)", "1d interpolation matrix: " << P1d.toString(true) );

  tarch::la::DynamicMatrix P2d( P1d, P1d, false);
  logDebug( "createLinearInterpolationMatrix(...)", "2d interpolation matrix: " << P2d.toString(true) );

  logTraceOut( "createLinearInterpolationMatrix(...)" );
  #if Dimensions==3
  return new tarch::la::DynamicMatrix( P2d, P1d, false);
  #else
  return new tarch::la::DynamicMatrix( P2d );
  #endif
}


void toolbox::blockstructured::internal::projectInterpolatedFineCellsOnHaloLayer_AoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                fineGridCellValuesLeft,
  const double* __restrict__                fineGridCellValuesRight,
  double*                                   fineGridFaceValues
) {
  logTraceInWith3Arguments( "projectInterpolatedFineCellsOnHaloLayer_AoS(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap );

  const int    normal        = marker.getSelectedFaceNumber() % Dimensions;

  dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
    for (int iFine=0;   iFine<overlap;   iFine++) {
      tarch::la::Vector<Dimensions,int> fineVolumeLeftDest = kFine;
      tarch::la::Vector<Dimensions,int> fineVolumeLeftSrc  = kFine;

      fineVolumeLeftDest(normal) = iFine;
      fineVolumeLeftSrc(normal)  = numberOfDoFsPerAxisInPatch - overlap + iFine;

      tarch::la::Vector<Dimensions,int> fineVolumeRightDest = kFine;
      tarch::la::Vector<Dimensions,int> fineVolumeRightSrc  = kFine;

      fineVolumeRightDest(normal) = iFine + overlap;
      fineVolumeRightSrc(normal)  = iFine;

      int fineVolumeLeftDestLinearised = serialisePatchIndexInOverlap(
        fineVolumeLeftDest,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      int fineVolumeRightDestLinearised = serialisePatchIndexInOverlap(
        fineVolumeRightDest,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      int fineVolumeLeftSrcLinearised = peano4::utils::dLinearised(
        fineVolumeLeftSrc,
        numberOfDoFsPerAxisInPatch
      );
      int fineVolumeRightSrcLinearised = peano4::utils::dLinearised(
        fineVolumeRightSrc,
        numberOfDoFsPerAxisInPatch
      );

      for (int j=0; j<unknowns; j++) {
        fineGridFaceValues[fineVolumeLeftDestLinearised*unknowns+j]  = fineGridCellValuesLeft[fineVolumeLeftSrcLinearised*unknowns+j];
        fineGridFaceValues[fineVolumeRightDestLinearised*unknowns+j] = fineGridCellValuesRight[fineVolumeRightSrcLinearised*unknowns+j];
      }
    }
  }

  logTraceOut( "projectInterpolatedFineCellsOnHaloLayer_AoS(...)" );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap  faceInterpolationMap;
  internal::FaceInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch,overlap,normal);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( faceInterpolationMap.count(key)==0 ) {
    faceInterpolationMap[key] = internal::createPiecewiseConstantInterpolationMatrix(numberOfDoFsPerAxisInPatch,normal,overlap);
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*2*overlap;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2*overlap;
  #endif

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      3, 1, normal
    )
  );

  logTraceOut( "interpolateHaloLayer_AoS_piecewise_constant(...)" );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  if ( marker.isInteriorFaceWithinPatch() ) {
    #if Dimensions==2
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #else
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #endif

    double* leftAdjacentPatchData  = tarch::allocateMemory(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory(patchSize * unknowns, tarch::MemoryLocation::Heap);

    tarch::la::Vector<Dimensions,int> leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions,int> rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex( marker.getSelectedFaceNumber()%Dimensions )--;

    internal::interpolateCell_AoS_piecewise_constant(
      leftAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      leftAdjacentPatchData
    );
    internal::interpolateCell_AoS_piecewise_constant(
      rightAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      rightAdjacentPatchData
    );

    internal::projectInterpolatedFineCellsOnHaloLayer_AoS(
      marker, numberOfDoFsPerAxisInPatch, overlap, unknowns,
      leftAdjacentPatchData, rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData,  tarch::MemoryLocation::Heap );
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap );
  }
  else {
    interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_piecewise_constant(...)" );
}



void toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );

  internal::interpolateCell_AoS_piecewise_constant(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues
  );

  logTraceOut("interpolateCell_AoS_piecewise_constant(...)");
}


void toolbox::blockstructured::internal::interpolateCell_AoS_piecewise_constant(
  const tarch::la::Vector<Dimensions,int>&  relativePositionWithinFatherCell,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  static internal::CellInterpolationMap  cellInterpolationMap;
  internal::CellInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( cellInterpolationMap.count(key)==0 ) {
    cellInterpolationMap[key] = internal::createPiecewiseConstantInterpolationMatrix(numberOfDoFsPerAxisInPatch);
  }
  tarch::la::DynamicMatrix* P = cellInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
  #endif

  P->batchedMultiplyAoS(
    fineGridCellValues,    // image
    coarseGridCellValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialiseMarkerIn3x3PatchAssembly(relativePositionWithinFatherCell, numberOfDoFsPerAxisInPatch)
  );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_linear(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
/*  assertion(overlap==1);

  const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;

  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...)", numberOfDoFsPerAxisInPatch, overlap, unknowns, normal );

  #if Dimensions==2
  if ( normal==0 ) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,0) );
    const int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(1);
    const int firstRowInP    = matrixRowBlock * numberOfDoFsPerAxisInPatch * 2;
    logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...) - a", matrixRowBlock, firstRowInP, P.toString(), P.vectorToString(coarseGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      firstRowInP
    );
    logTraceOutWith1Argument( "interpolateHaloLayer_AoS_linear(...) - a", P.vectorToString(fineGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
  }
  else if (normal==1) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,1) );
    const int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0);
    const int firstRowInP    = matrixRowBlock * numberOfDoFsPerAxisInPatch * 2;
    logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...) - b", matrixRowBlock, firstRowInP, P.toString(), P.vectorToString(coarseGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      firstRowInP
    );
    logTraceOutWith1Argument( "interpolateHaloLayer_AoS_linear(...) - b", P.vectorToString(fineGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
  }
  #elif Dimensions==3
  if ( normal==0 ) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,0) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(1) + marker.getRelativePositionWithinFatherCell()(2)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  else if (normal==1) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,1) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0) + marker.getRelativePositionWithinFatherCell()(2)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  else if (normal==2) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,2) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0) + marker.getRelativePositionWithinFatherCell()(1)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  #endif

  logTraceOut( "interpolateHaloLayer_AoS_linear(...)")*/
}

/*

tarch::la::DynamicMatrix  toolbox::blockstructured::internal::createLinearInterpolationMatrix(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch
) {
  tarch::la::DynamicMatrix Project = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch);

  #if Dimensions==3
  tarch::la::DynamicMatrix Pick = tarch::la::DynamicMatrix::id( numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  int interleaveX =2*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveX, numberOfDoFsPerAxisInPatch, numberOfDoFsPerAxisInPatch);
  int interleaveY =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveY, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  int interleaveZ =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveZ, 9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(0)*1*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(1)*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(2)*9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  #else
  tarch::la::DynamicMatrix Pick = tarch::la::DynamicMatrix::id( numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  int interleaveX = 2*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveX, numberOfDoFsPerAxisInPatch, numberOfDoFsPerAxisInPatch);
  int interleaveY =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveY, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(0)*1*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(1)*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  #endif

  tarch::la::DynamicMatrix PickTimesProject = Pick * Project;
  return PickTimesProject;
}
*/


void toolbox::blockstructured::interpolateCell_AoS_linear(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
/*
  tarch::la::DynamicMatrix PickTimesProject = internal::createLinearInterpolationMatrix(marker,numberOfDoFsPerAxisInPatch);

  #if Dimensions==2
  PickTimesProject.batchedMultiplyAoS(
    fineGridValues, // image
    coarseGridValues,  // preimage
    unknowns,          // batch size, i.e. how often to apply it in one AoS rush
    numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, // result size, i.e. size of image
    0
  );
  #else
  PickTimesProject.batchedMultiplyAoS(
    fineGridValues, // image
    coarseGridValues,  // preimage
    unknowns,          // batch size, i.e. how often to apply it in one AoS rush
    numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, // result size, i.e. size of image
    0
  );
  #endif
*/
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_linear(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridFaceValues,
  double*                                   coarseGridFaceValues,
  double*                                   coarseGridCellValues
) {
/*  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  int normal = marker.getSelectedFaceNumber() % Dimensions;
  if (
    marker.getRelativePositionWithinFatherCell()(normal)==0
    or
    marker.getRelativePositionWithinFatherCell()(normal)==3
  ) {
    interpolateHaloLayer_AoS_linear(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      fineGridFaceValues,
      coarseGridFaceValues
    );
  }
  else {
    tarch::la::DynamicMatrix Project = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch);

    #if Dimensions==2
    double* interpolatedValues = tarch::allocateMemory(3*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns,tarch::MemoryLocation::Heap);
    Project.batchedMultiplyAoS(
      interpolatedValues, // image
      coarseGridCellValues,  // preimage
      unknowns
    );
    #else
    double* interpolatedValues = tarch::allocateMemory(3*3*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns,tarch::MemoryLocation::Heap);
    Project.batchedMultiplyAoS(
      interpolatedValues, // image
      coarseGridCellValues,  // preimage
      unknowns
    );
    #endif

    internal::projectInterpolatedFineCellsOnHaloLayer_AoS(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap, unknowns,
      fineGridFaceValues,
      interpolatedValues
    );

    tarch::freeMemory(interpolatedValues,tarch::MemoryLocation::Heap);
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear(...)" );*/
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_outflow(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridFaceValues,
  double*                                   coarseGridFaceValues,
  double*                                   coarseGridCellValues
) {
  interpolateHaloLayer_AoS_piecewise_constant(marker,numberOfDoFsPerAxisInPatch,overlap,unknowns,fineGridFaceValues,coarseGridFaceValues,coarseGridCellValues);
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_outflow(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  assertionEquals(overlap,1);

  interpolateHaloLayer_AoS_linear(
    marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues
  );

  const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
  const bool pickLeftHalfOfHaloOnFineGrid  = (marker.getSelectedFaceNumber() < Dimensions);

  dfore(fineVolumeOutside,numberOfDoFsPerAxisInPatch,normal,pickLeftHalfOfHaloOnFineGrid ? 0 : 1) {
    tarch::la::Vector<Dimensions,int>    fineVolumeInside = fineVolumeOutside;
    fineVolumeInside(normal) = pickLeftHalfOfHaloOnFineGrid ? 1 : 0;

    int fineVolumeOutsideLinearised = serialisePatchIndexInOverlap(
      fineVolumeOutside,
      numberOfDoFsPerAxisInPatch, overlap, normal
      );
    int fineVolumeInsideLinearised = serialisePatchIndexInOverlap(
      fineVolumeInside,
      numberOfDoFsPerAxisInPatch, overlap, normal
      );

    for (int j=0; j<unknowns; j++) {
      fineGridValues[fineVolumeOutsideLinearised*unknowns+j] = fineGridValues[fineVolumeInsideLinearised*unknowns+j];
    }
  }
}


void toolbox::blockstructured::interpolateCell_AoS_outflow(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
    marker,
    numberOfDoFsPerAxisInPatch,
    unknowns,
    fineGridValues,
    coarseGridValues
  );
}


