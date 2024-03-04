#include "Interpolation.h"
#include "Enumeration.h"

#include "peano4/utils/Loop.h"
#include "tarch/la/DynamicMatrix.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  [[maybe_unused]] tarch::logging::Log _log( "toolbox::blockstructured" );

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

  logDebug( "createPiecewiseConstantInterpolationMatrix(int)", "created new volumetric matrix for " << numberOfDoFsPerAxisInPatch << ": " << result->toString() );
  return result;
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::createPiecewiseConstantInterpolationMatrix(
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int normal,
  [[maybe_unused]] int overlap
) {
  tarch::la::DynamicMatrix P1d(3,1,{ {1.0},{1.0},{1.0} });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );

  return createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
    P1d, numberOfDoFsPerAxisInPatch, normal
  );
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
  const tarch::la::DynamicMatrix&  P1d,
  int                              numberOfDoFsPerAxisInPatch,
  int                              normal
) {
  assertionEquals1( P1d.cols(),   numberOfDoFsPerAxisInPatch, P1d.toString() );
  assertionEquals1( P1d.rows(), 3*numberOfDoFsPerAxisInPatch, P1d.toString() );

  #if Dimensions==3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d, P1d, false);
  #else
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d );
  #endif

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

  P->insertEmptyColumns(pattern,pattern,pattern);
  P->replicateRows( pattern, 2, pattern, false );
  logDebug( "createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(...)", "matrix for normal=" << normal << ": " << P->toString() );
  return P;
}

tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
  const tarch::la::DynamicMatrix&  P1d,
  int                              numberOfDoFsPerAxisInPatch,
  int                              normal
) {
  assertionEquals1( P1d.cols(),   numberOfDoFsPerAxisInPatch, P1d.toString() );
  assertionEquals1( P1d.rows(), 3*numberOfDoFsPerAxisInPatch, P1d.toString() );

  #if Dimensions==3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d, P1d, false);
  #else
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix( P1d );
  #endif

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

  P->scale(0.5);

  // Split matrix into blocks of pattern columns and copy each block once.
  // Matrix grows by factor of two which reflects the fact that the overlap
  // is of size 1.
  P->replicateCols( pattern, 2, 0, false );
  P->replicateRows( pattern, 2, 0, false );
  logDebug( "createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(...)", "matrix for normal=" << normal << ": " << P->toString() );
  return P;
}

tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createLinearInterpolationMatrix(
  int numberOfDoFsPerAxisInPatch,
  int normal,
  bool extrapolateLinearly,
  bool interpolateLinearlyBetweenRealAndRestrictedVolumes
) {
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  if (extrapolateLinearly) {
    P1d(0,0) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(0,1) = -1.0/3.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) = -1.0/3.0;
  }
  else {
    P1d(0,0) =  1.0;
    P1d(0,1) =  0.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;
  }

  logDebug( "createLinearInterpolationMatrix(...)", "1d matrix: " << P1d.toString() );

  if (interpolateLinearlyBetweenRealAndRestrictedVolumes) {
    return createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
      P1d, numberOfDoFsPerAxisInPatch, normal
    );
  }
  else {
    return createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
      P1d, numberOfDoFsPerAxisInPatch, normal
    );
  }
}

tarch::la::DynamicMatrix*  toolbox::blockstructured::internal::createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch, bool extrapolateLinearly) {
  logTraceInWith1Argument( "createLinearInterpolationMatrix(...)", numberOfDoFsPerAxisInPatch );
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  if (extrapolateLinearly) {
    P1d(0,0) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(0,1) = -1.0/3.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) = -1.0/3.0;
  }
  else {
    P1d(0,0) =  1.0;
    P1d(0,1) =  0.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;
  }

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

      int fineVolumeLeftDestLinearised = serialiseVoxelIndexInOverlap(
        fineVolumeLeftDest,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      int fineVolumeRightDestLinearised = serialiseVoxelIndexInOverlap(
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

//
// ==========================================
// Piece-wise constant interpolation routines
// ==========================================
//
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

  logDebug( "interpolateHaloLayer_AoS_piecewise_constant(...)", marker.toString() << ": " << P->toString() );

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
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

    double* leftAdjacentPatchData  = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);

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

//
// =============================
// Linear interpolation routines
// =============================
//
void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap  faceInterpolationMap;
  internal::FaceInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch,overlap,normal);

  assertionEquals(overlap,1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( faceInterpolationMap.count(key)==0 ) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,normal,false,false);
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*2*overlap;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2*overlap;
  #endif

  logDebug( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)", marker.toString() << ": " << P->toString() );

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)" );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  if ( marker.isInteriorFaceWithinPatch() ) {
    #if Dimensions==2
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #else
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #endif

    double* leftAdjacentPatchData  = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);

    tarch::la::Vector<Dimensions,int> leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions,int> rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex( marker.getSelectedFaceNumber()%Dimensions )--;

    internal::interpolateCell_AoS_linear(
      leftAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      leftAdjacentPatchData,
      false
    );
    internal::interpolateCell_AoS_linear(
      rightAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      rightAdjacentPatchData,
      false
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
    interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)" );
}

void toolbox::blockstructured::interpolateCell_AoS_linear_with_constant_extrapolation(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_linear_with_constant_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    false
  );

  logTraceOut("interpolateCell_AoS_linear_with_constant_extrapolation(...)");
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap  faceInterpolationMap;
  internal::FaceInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch,overlap,normal);

  assertionEquals(overlap,1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( faceInterpolationMap.count(key)==0 ) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,normal,true,false);
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*2*overlap;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2*overlap;
  #endif

  logDebug( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)", marker.toString() << ": " << P->toString() );

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)" );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  if ( marker.isInteriorFaceWithinPatch() ) {
    #if Dimensions==2
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #else
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #endif

    double* leftAdjacentPatchData  = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);

    tarch::la::Vector<Dimensions,int> leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions,int> rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex( marker.getSelectedFaceNumber()%Dimensions )--;

    internal::interpolateCell_AoS_linear(
      leftAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      leftAdjacentPatchData,
      true
    );
    internal::interpolateCell_AoS_linear(
      rightAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      rightAdjacentPatchData,
      true
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
    interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)" );
}

void toolbox::blockstructured::interpolateCell_AoS_linear_with_linear_extrapolation(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_linear_with_linear_extrapolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    true
  );

  logTraceOut("interpolateCell_AoS_linear_with_linear_extrapolation(...)");
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap  faceInterpolationMap;
  internal::FaceInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch,overlap,normal);

  assertionEquals(overlap,1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( faceInterpolationMap.count(key)==0 ) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,normal,false,true);
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*2*overlap;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2*overlap;
  #endif

  logDebug( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)", marker.toString() << ": " << P->toString() );

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)" );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  if ( marker.isInteriorFaceWithinPatch() ) {
    #if Dimensions==2
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #else
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #endif

    double* leftAdjacentPatchData  = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);

    tarch::la::Vector<Dimensions,int> leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions,int> rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex( marker.getSelectedFaceNumber()%Dimensions )--;

    internal::interpolateCell_AoS_linear(
      leftAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      leftAdjacentPatchData,
      false
    );
    internal::interpolateCell_AoS_linear(
      rightAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      rightAdjacentPatchData,
      false
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
    interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)" );
}

void toolbox::blockstructured::interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    false
  );

  logTraceOut("interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)");
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap  faceInterpolationMap;
  internal::FaceInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch,overlap,normal);

  assertionEquals(overlap,1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( faceInterpolationMap.count(key)==0 ) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,normal,true,true);
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

  #if Dimensions==2
  const int patchSize = numberOfDoFsPerAxisInPatch*2*overlap;
  #else
  const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2*overlap;
  #endif

  logDebug( "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)", marker.toString() << ": " << P->toString() );

  P->batchedMultiplyAoS(
    fineGridFaceValues,    // image
    coarseGridFaceValues,  // preimage
    unknowns,              // batch size, i.e. how often to apply it in one AoS rush
    patchSize,             // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)" );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  if ( marker.isInteriorFaceWithinPatch() ) {
    #if Dimensions==2
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #else
    const int patchSize = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
    #endif

    double* leftAdjacentPatchData  = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(patchSize * unknowns, tarch::MemoryLocation::Heap);

    tarch::la::Vector<Dimensions,int> leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions,int> rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex( marker.getSelectedFaceNumber()%Dimensions )--;

    internal::interpolateCell_AoS_linear(
      leftAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      leftAdjacentPatchData,
      true
    );
    internal::interpolateCell_AoS_linear(
      rightAdjacentPatchIndex,
      numberOfDoFsPerAxisInPatch,
      unknowns,
      coarseGridCellValues,
      rightAdjacentPatchData,
      true
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
    interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)" );
}


void toolbox::blockstructured::interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
  int                                       numberOfDoFsPerAxisInSourcePatch,
  int                                       numberOfDoFsPerAxisInDestinationPatch,
  int                                       haloSourcePatch,
  int                                       haloDestinationPatch,
  int                                       unknowns,
  const double* __restrict__                sourceValues,
  double* __restrict__                      destinationValues,
  ::peano4::utils::LoopPlacement          parallelisation
) {
  assertion( numberOfDoFsPerAxisInDestinationPatch>numberOfDoFsPerAxisInSourcePatch );

  simtDforWithSchedulerInstructions( destinationVolume, numberOfDoFsPerAxisInDestinationPatch+2*haloDestinationPatch, parallelisation )
    const int baseIndexDestination = peano4::utils::dLinearised(destinationVolume,numberOfDoFsPerAxisInDestinationPatch+2*haloDestinationPatch) * unknowns;
    for (int unknown=0; unknown<unknowns; unknown++) {
      destinationValues[baseIndexDestination+unknown] = 0.0;
    }

    dfor( sourceVolume, numberOfDoFsPerAxisInSourcePatch+2*haloSourcePatch ) {
      tarch::la::Vector<Dimensions, double> normalisedSourcePosition =
          tarch::la::multiplyComponents(
            tarch::la::Vector<Dimensions, double>(1.0/numberOfDoFsPerAxisInSourcePatch),
            tarch::la::convertScalar<double>(sourceVolume) + tarch::la::Vector<Dimensions, double>(0.5 - haloSourcePatch)
          );
      tarch::la::Vector<Dimensions, double> normalisedDestinationPosition =
          tarch::la::multiplyComponents(
            tarch::la::Vector<Dimensions, double>(1.0/numberOfDoFsPerAxisInDestinationPatch),
            tarch::la::convertScalar<double>(destinationVolume) + tarch::la::Vector<Dimensions, double>(0.5 - haloDestinationPatch)
          );

      int outsidePatchAlongCoordinateAxis = 0;
      for (int d=0; d<Dimensions; d++) {
//        outsidePatchAlongCoordinateAxis +=  ( sourceVolume(d) <  haloSourcePatch ) ? 1 : 0;
//        outsidePatchAlongCoordinateAxis +=  ( sourceVolume(d) >= haloSourcePatch+numberOfDoFsPerAxisInSourcePatch ) ? 1 : 0;
        if ( sourceVolume(d) <  haloSourcePatch )                                   outsidePatchAlongCoordinateAxis++;
        if ( sourceVolume(d) >= haloSourcePatch+numberOfDoFsPerAxisInSourcePatch )  outsidePatchAlongCoordinateAxis++;
      }
      const bool isFromUnitialisedDiagonal = outsidePatchAlongCoordinateAxis>1;

      double weight = 1.0;
      for (int d=0; d<Dimensions; d++) {
        double distanceAlongCoordinateAxis = std::abs( normalisedDestinationPosition(d) - normalisedSourcePosition(d) );
        double h = 1.0 / static_cast<double>(numberOfDoFsPerAxisInSourcePatch);
        distanceAlongCoordinateAxis = 1.0 - distanceAlongCoordinateAxis / h ;
        weight *= std::max(0.0, distanceAlongCoordinateAxis);
      }
      #if !defined(SharedSYCL)
      assertion( tarch::la::greaterEquals(weight, 0.0) );
      assertion( tarch::la::smallerEquals(weight, 1.0) );
      #endif

      tarch::la::Vector<Dimensions, int> amendedSourceVolume = sourceVolume;
      if (isFromUnitialisedDiagonal) {
        #pragma unroll
        for (int d=0; d<Dimensions; d++) {
          amendedSourceVolume(d) = std::max( amendedSourceVolume(d), haloSourcePatch );
          amendedSourceVolume(d) = std::min( amendedSourceVolume(d), haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1 );
        }
      }

      int baseIndexSource = peano4::utils::dLinearised(amendedSourceVolume, numberOfDoFsPerAxisInSourcePatch+2*haloSourcePatch) * unknowns;
      for (int unknown=0; unknown<unknowns; unknown++) {
        destinationValues[baseIndexDestination+unknown] += weight * sourceValues[baseIndexSource+unknown];
      }
    }
  endSimtDfor
}

void toolbox::blockstructured::interpolateCellDataAssociatedToVolumesIntoOverlappingCell_fourthOrder(
  int                                       numberOfDoFsPerAxisInSourcePatch,
  int                                       numberOfDoFsPerAxisInDestinationPatch,
  int                                       haloSourcePatch,
  int                                       haloDestinationPatch,
  int                                       unknowns,
  const double* __restrict__                sourceValues,
  double* __restrict__                      destinationValues,
  ::peano4::utils::LoopPlacement          parallelisation
) {
  assertion( numberOfDoFsPerAxisInDestinationPatch>numberOfDoFsPerAxisInSourcePatch );

  simtDforWithSchedulerInstructions( destinationVolume, numberOfDoFsPerAxisInDestinationPatch+2*haloDestinationPatch, parallelisation )
    const int baseIndexDestination = peano4::utils::dLinearised(destinationVolume,numberOfDoFsPerAxisInDestinationPatch+2*haloDestinationPatch) * unknowns;
    for (int unknown=0; unknown<unknowns; unknown++) {
      destinationValues[baseIndexDestination+unknown] = 0.0;
    }
  endSimtDfor
}



void toolbox::blockstructured::interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    true
  );

  logTraceOut("interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)");
}

void toolbox::blockstructured::internal::interpolateCell_AoS_linear(
  const tarch::la::Vector<Dimensions,int>&  relativePositionWithinFatherCell,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__                coarseGridCellValues,
  double* __restrict__                      fineGridCellValues,
  bool                                      extrapolateLinearly
) {
  static internal::CellInterpolationMap  cellInterpolationMap;
  internal::CellInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if ( cellInterpolationMap.count(key)==0 ) {
    cellInterpolationMap[key] = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,extrapolateLinearly);
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

void toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                normalInterpolationMatrix1d,
  const double* __restrict__                tangentialInterpolationMatrix1d,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  logTraceInWith3Arguments( "interpolateHaloLayer_AoS_tensor_product(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap );

  const int    normal          = marker.getSelectedFaceNumber() % Dimensions;

  dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
    for (int iFine=0;   iFine<overlap;   iFine++) {
      tarch::la::Vector<Dimensions,int> dest = kFine;
      dest(normal) = (marker.getSelectedFaceNumber() < Dimensions) ? iFine : 2 * overlap - iFine - 1;

      int destLinearised = serialiseVoxelIndexInOverlap(
        dest,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );

      logDebug( "interpolateHaloLayer_AoS_tensor_product(...)", "clear dof " << dest );
      for (int unknown=0; unknown<unknowns; unknown++) {
        fineGridFaceValues[destLinearised*unknowns+unknown] = 0.0;
      }

      dfore(kCoarse,numberOfDoFsPerAxisInPatch,normal,0) {
        for (int iCoarse=0;   iCoarse<2*overlap;   iCoarse++) {
          tarch::la::Vector<Dimensions,int> src  = kCoarse;
          src(normal)  = (marker.getSelectedFaceNumber() < Dimensions) ?  iCoarse : 2 * overlap - iCoarse - 1;
          int srcLinearised = serialiseVoxelIndexInOverlap(
            src,
            numberOfDoFsPerAxisInPatch, overlap, normal
          );

          double weight = 1.0;
          for (int d=0; d<Dimensions; d++) {
            if (d==normal) {
              int col   = iCoarse;
              int row   = iFine;

              assertion4(col>=0,row,col,src,dest);
              assertion4(row>=0,row,col,src,dest);
              int index = col + row * 2 * overlap;
              logDebug( "interpolateHaloLayer_AoS_tensor_product(...)", "(" << row << "," << col << ")=" << index << " (normal contribution)");
              weight *= normalInterpolationMatrix1d[ index ];
              assertion(weight==weight);
            }
            else {
              int col   = src(d);
              int row   = dest(d) + marker.getRelativePositionWithinFatherCell()(d) * numberOfDoFsPerAxisInPatch;
              assertion4(col>=0,row,col,src,dest);
              assertion4(row>=0,row,col,src,dest);
              int index = col + row * numberOfDoFsPerAxisInPatch;
              logDebug( "interpolateHaloLayer_AoS_tensor_product(...)", "(" << row << "," << col << ")=" << index << " (tangential contribution)" );
              weight *= tangentialInterpolationMatrix1d[ index ];
              assertion(weight==weight);
            }
          }

          logDebug( "interpolateHaloLayer_AoS_tensor_product(...)", "add dof " << src << " to " << dest << " with weight " << weight );

          for (int unknown=0; unknown<unknowns; unknown++) {
            fineGridFaceValues[destLinearised*unknowns+unknown] += weight * coarseGridFaceValues[srcLinearised*unknowns+unknown];
          }
        }
      }
    }
  }

  logTraceOut( "interpolateHaloLayer_AoS_tensor_product(...)" );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int                                       numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int                                       overlap,
  [[maybe_unused]] int                                       unknowns,
  [[maybe_unused]] const double* __restrict__                normalInterpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__                tangentialInterpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__                coarseGridCellValues,
  [[maybe_unused]] const double* __restrict__                coarseGridFaceValues,
  [[maybe_unused]] double* __restrict__                      fineGridFaceValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateCell_AoS_tensor_product(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int                                       numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int                                       unknowns,
  [[maybe_unused]] const double* __restrict__                interpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__                coarseGridCellValues,
  [[maybe_unused]] double* __restrict__                      fineGridCellValues
) {
  assertion(false);
}
