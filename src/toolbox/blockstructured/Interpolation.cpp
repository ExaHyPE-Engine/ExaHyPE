#include "Interpolation.h"

#include <vector>

#include "Enumeration.h"
#include "peano4/utils/Loop.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/DynamicMatrixOperations.h"
#include "tarch/la/GramSchmidt.h"
#include "tarch/la/LUDecomposition.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"

namespace {
  [[maybe_unused]] tarch::logging::Log _log("toolbox::blockstructured");

  tarch::multicore::BooleanSemaphore interpolationMapSemaphore;
} // namespace

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch) {
#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
  const int numberOfPatches = 3 * 3;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * numberOfDoFsPerAxisInPatch;
  const int                 numberOfPatches = 3 * 3 * 3;
#endif

  tarch::la::DynamicMatrix* result = new tarch::la::DynamicMatrix(
    patchSize * numberOfPatches,
    patchSize
  );

  int currentRow = 0;
  dfor3(patchIndex) dfor(volumeIndex, numberOfDoFsPerAxisInPatch) {
    tarch::la::Vector<Dimensions, int>
      sourceCell = (volumeIndex + patchIndex * numberOfDoFsPerAxisInPatch) / 3;
    int sourceCellLinearised = peano4::utils::dLinearised(
      sourceCell,
      numberOfDoFsPerAxisInPatch
    );
    (*result)(currentRow, sourceCellLinearised) = 1.0;
    currentRow++;
  }
  enddforx

    logDebug(
      "createPiecewiseConstantInterpolationMatrix(int)",
      "created new volumetric matrix for "
        << numberOfDoFsPerAxisInPatch << ": " << result->toString()
    );
  return result;
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createPiecewiseConstantInterpolationMatrix(
    [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
    [[maybe_unused]] int normal,
    [[maybe_unused]] int overlap
  ) {
  tarch::la::DynamicMatrix P1d(3, 1, {{1.0}, {1.0}, {1.0}});
  P1d.replicateRows(3, numberOfDoFsPerAxisInPatch, 1, true);

  return createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
    P1d,
    numberOfDoFsPerAxisInPatch,
    normal
  );
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
    const tarch::la::DynamicMatrix& P1d,
    int                             numberOfDoFsPerAxisInPatch,
    int                             normal
  ) {
  assertionEquals1(P1d.cols(), numberOfDoFsPerAxisInPatch, P1d.toString());
  assertionEquals1(P1d.rows(), 3 * numberOfDoFsPerAxisInPatch, P1d.toString());

#if Dimensions == 3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d, P1d, false);
#else
  tarch::la::DynamicMatrix* P               = new tarch::la::DynamicMatrix(P1d);
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
    pattern = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
    break;
  }

  P->insertEmptyColumns(pattern, pattern, pattern);
  P->replicateRows(pattern, 2, pattern, false);
  logDebug(
    "createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(...)",
    "matrix for normal=" << normal << ": " << P->toString()
  );
  return P;
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
    const tarch::la::DynamicMatrix& P1d,
    int                             numberOfDoFsPerAxisInPatch,
    int                             normal
  ) {
  assertionEquals1(P1d.cols(), numberOfDoFsPerAxisInPatch, P1d.toString());
  assertionEquals1(P1d.rows(), 3 * numberOfDoFsPerAxisInPatch, P1d.toString());

#if Dimensions == 3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d, P1d, false);
#else
  tarch::la::DynamicMatrix* P               = new tarch::la::DynamicMatrix(P1d);
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
    pattern = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
    break;
  }

  P->scale(0.5);

  // Split matrix into blocks of pattern columns and copy each block once.
  // Matrix grows by factor of two which reflects the fact that the overlap
  // is of size 1.
  P->replicateCols(pattern, 2, 0, false);
  P->replicateRows(pattern, 2, 0, false);
  logDebug(
    "createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(...)",
    "matrix for normal=" << normal << ": " << P->toString()
  );
  return P;
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createLinearInterpolationMatrix(
    int  numberOfDoFsPerAxisInPatch,
    int  normal,
    bool extrapolateLinearly,
    bool interpolateLinearlyBetweenRealAndRestrictedVolumes
  ) {
  tarch::la::DynamicMatrix P1d(
    3,
    3,
    {{1.0 / 3.0, 2.0 / 3.0, 0.0},
     {0.0, 3.0 / 3.0, 0.0},
     {0.0, 2.0 / 3.0, 1.0 / 3.0}}
  );
  P1d.replicateRows(3, numberOfDoFsPerAxisInPatch, 1, true);
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  if (extrapolateLinearly) {
    P1d(0, 0) = 2.0 / 3.0 + 1.0 / 3.0 * 2.0;
    P1d(0, 1) = -1.0 / 3.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 1
    ) = 2.0 / 3.0 + 1.0 / 3.0 * 2.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 2
    ) = -1.0 / 3.0;
  } else {
    P1d(0, 0) = 1.0;
    P1d(0, 1) = 0.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 1
    ) = 1.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 2
    ) = 0.0;
  }

  logDebug(
    "createLinearInterpolationMatrix(...)",
    "1d matrix: " << P1d.toString()
  );

  if (interpolateLinearlyBetweenRealAndRestrictedVolumes) {
    return createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
      P1d,
      numberOfDoFsPerAxisInPatch,
      normal
    );
  } else {
    return createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
      P1d,
      numberOfDoFsPerAxisInPatch,
      normal
    );
  }
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createLinearInterpolationMatrix(
    int  numberOfDoFsPerAxisInPatch,
    bool extrapolateLinearly
  ) {
  logTraceInWith1Argument(
    "createLinearInterpolationMatrix(...)",
    numberOfDoFsPerAxisInPatch
  );
  tarch::la::DynamicMatrix P1d(
    3,
    3,
    {{1.0 / 3.0, 2.0 / 3.0, 0.0},
     {0.0, 3.0 / 3.0, 0.0},
     {0.0, 2.0 / 3.0, 1.0 / 3.0}}
  );
  P1d.replicateRows(3, numberOfDoFsPerAxisInPatch, 1, true);
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  if (extrapolateLinearly) {
    P1d(0, 0) = 2.0 / 3.0 + 1.0 / 3.0 * 2.0;
    P1d(0, 1) = -1.0 / 3.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 1
    ) = 2.0 / 3.0 + 1.0 / 3.0 * 2.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 2
    ) = -1.0 / 3.0;
  } else {
    P1d(0, 0) = 1.0;
    P1d(0, 1) = 0.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 1
    ) = 1.0;
    P1d(
      numberOfDoFsPerAxisInPatch * 3 - 1,
      numberOfDoFsPerAxisInPatch - 2
    ) = 0.0;
  }

  logDebug(
    "createLinearInterpolationMatrix(...)",
    "1d interpolation matrix: " << P1d.toString(true)
  );

  tarch::la::DynamicMatrix P2d(P1d, P1d, false);
  logDebug(
    "createLinearInterpolationMatrix(...)",
    "2d interpolation matrix: " << P2d.toString(true)
  );

  logTraceOut("createLinearInterpolationMatrix(...)");
#if Dimensions == 3
  return new tarch::la::DynamicMatrix(P2d, P1d, false);
#else
  return new tarch::la::DynamicMatrix(P2d);
#endif
}

void toolbox::blockstructured::internal::
  projectInterpolatedFineCellsOnHaloLayer_AoS(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ fineGridCellValuesLeft,
    const double* __restrict__ fineGridCellValuesRight,
    double* fineGridFaceValues
  ) {
  logTraceInWith3Arguments(
    "projectInterpolatedFineCellsOnHaloLayer_AoS(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  dfore(kFine, numberOfDoFsPerAxisInPatch, normal, 0) {
    for (int iFine = 0; iFine < overlap; iFine++) {
      tarch::la::Vector<Dimensions, int> fineVolumeLeftDest = kFine;
      tarch::la::Vector<Dimensions, int> fineVolumeLeftSrc  = kFine;

      fineVolumeLeftDest(normal) = iFine;
      fineVolumeLeftSrc(normal)  = numberOfDoFsPerAxisInPatch - overlap + iFine;

      tarch::la::Vector<Dimensions, int> fineVolumeRightDest = kFine;
      tarch::la::Vector<Dimensions, int> fineVolumeRightSrc  = kFine;

      fineVolumeRightDest(normal) = iFine + overlap;
      fineVolumeRightSrc(normal)  = iFine;

      int fineVolumeLeftDestLinearised = serialiseVoxelIndexInOverlap(
        fineVolumeLeftDest,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );
      int fineVolumeRightDestLinearised = serialiseVoxelIndexInOverlap(
        fineVolumeRightDest,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );
      int fineVolumeLeftSrcLinearised = peano4::utils::dLinearised(
        fineVolumeLeftSrc,
        numberOfDoFsPerAxisInPatch
      );
      int fineVolumeRightSrcLinearised = peano4::utils::dLinearised(
        fineVolumeRightSrc,
        numberOfDoFsPerAxisInPatch
      );

      for (int j = 0; j < unknowns; j++) {
        fineGridFaceValues[fineVolumeLeftDestLinearised * unknowns + j]
          = fineGridCellValuesLeft[fineVolumeLeftSrcLinearised * unknowns + j];
        fineGridFaceValues[fineVolumeRightDestLinearised * unknowns + j]
          = fineGridCellValuesRight
            [fineVolumeRightSrcLinearised * unknowns + j];
      }
    }
  }

  logTraceOut("projectInterpolatedFineCellsOnHaloLayer_AoS(...)");
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
  const double* __restrict__ coarseGridFaceValues,
  double* __restrict__ fineGridFaceValues
) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_piecewise_constant(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap faceInterpolationMap;
  internal::FaceInterpolationOperatorKey
    key(numberOfDoFsPerAxisInPatch, overlap, normal);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (faceInterpolationMap.count(key) == 0) {
    faceInterpolationMap[key] = internal::
      createPiecewiseConstantInterpolationMatrix(
        numberOfDoFsPerAxisInPatch,
        normal,
        overlap
      );
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * 2 * overlap;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * 2 * overlap;
#endif

  logDebug(
    "interpolateHaloLayer_AoS_piecewise_constant(...)",
    marker.toString() << ": " << P->toString()
  );

  P->batchedMultiplyAoS(
    fineGridFaceValues,   // image
    coarseGridFaceValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut("interpolateHaloLayer_AoS_piecewise_constant(...)");
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__ coarseGridCellValues,
  const double* __restrict__ coarseGridFaceValues,
  double* __restrict__ fineGridFaceValues
) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_piecewise_constant(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  if (marker.isInteriorFaceWithinPatch()) {
#if Dimensions == 2
    const int patchSize = numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#else
    const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#endif

    double* leftAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );

    tarch::la::Vector<Dimensions, int>
      leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions, int>
      rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex(marker.getSelectedFaceNumber() % Dimensions)--;

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
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      leftAdjacentPatchData,
      rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData, tarch::MemoryLocation::Heap);
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap);
  } else {
    interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut("interpolateHaloLayer_AoS_piecewise_constant(...)");
}

void toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__ coarseGridCellValues,
  double* __restrict__ fineGridCellValues
) {
  logTraceInWith3Arguments(
    "interpolateCell_AoS_piecewise_constant(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    unknowns
  );

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
  const tarch::la::Vector<Dimensions, int>& relativePositionWithinFatherCell,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__ coarseGridCellValues,
  double* __restrict__ fineGridCellValues
) {
  static internal::CellInterpolationMap  cellInterpolationMap;
  internal::CellInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (cellInterpolationMap.count(key) == 0) {
    cellInterpolationMap[key] = internal::
      createPiecewiseConstantInterpolationMatrix(numberOfDoFsPerAxisInPatch);
  }
  tarch::la::DynamicMatrix* P = cellInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * numberOfDoFsPerAxisInPatch;
#endif

  P->batchedMultiplyAoS(
    fineGridCellValues,   // image
    coarseGridCellValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialiseMarkerIn3x3PatchAssembly(
      relativePositionWithinFatherCell,
      numberOfDoFsPerAxisInPatch
    )
  );
}

//
// =============================
// Linear interpolation routines
// =============================
//
void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap faceInterpolationMap;
  internal::FaceInterpolationOperatorKey
    key(numberOfDoFsPerAxisInPatch, overlap, normal);

  assertionEquals(overlap, 1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (faceInterpolationMap.count(key) == 0) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(
      numberOfDoFsPerAxisInPatch,
      normal,
      false,
      false
    );
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * 2 * overlap;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * 2 * overlap;
#endif

  logDebug(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)",
    marker.toString() << ": " << P->toString()
  );

  P->batchedMultiplyAoS(
    fineGridFaceValues,   // image
    coarseGridFaceValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut("interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  if (marker.isInteriorFaceWithinPatch()) {
#if Dimensions == 2
    const int patchSize = numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#else
    const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#endif

    double* leftAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );

    tarch::la::Vector<Dimensions, int>
      leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions, int>
      rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex(marker.getSelectedFaceNumber() % Dimensions)--;

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
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      leftAdjacentPatchData,
      rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData, tarch::MemoryLocation::Heap);
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap);
  } else {
    interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut("interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateCell_AoS_linear_with_constant_extrapolation(
    const peano4::datamanagement::CellMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    double* __restrict__ fineGridCellValues
  ) {
  logTraceInWith3Arguments(
    "interpolateCell_AoS_linear_with_constant_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    unknowns
  );

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

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap faceInterpolationMap;
  internal::FaceInterpolationOperatorKey
    key(numberOfDoFsPerAxisInPatch, overlap, normal);

  assertionEquals(overlap, 1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (faceInterpolationMap.count(key) == 0) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(
      numberOfDoFsPerAxisInPatch,
      normal,
      true,
      false
    );
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * 2 * overlap;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * 2 * overlap;
#endif

  logDebug(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation(...)",
    marker.toString() << ": " << P->toString()
  );

  P->batchedMultiplyAoS(
    fineGridFaceValues,   // image
    coarseGridFaceValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut("interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)");
}

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  if (marker.isInteriorFaceWithinPatch()) {
#if Dimensions == 2
    const int patchSize = numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#else
    const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#endif

    double* leftAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );

    tarch::la::Vector<Dimensions, int>
      leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions, int>
      rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex(marker.getSelectedFaceNumber() % Dimensions)--;

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
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      leftAdjacentPatchData,
      rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData, tarch::MemoryLocation::Heap);
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap);
  } else {
    interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut("interpolateHaloLayer_AoS_linear_with_linear_extrapolation(...)");
}

void toolbox::blockstructured::
  interpolateCell_AoS_linear_with_linear_extrapolation(
    const peano4::datamanagement::CellMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    double* __restrict__ fineGridCellValues
  ) {
  logTraceInWith3Arguments(
    "interpolateCell_AoS_linear_with_linear_extrapolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    unknowns
  );

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

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap faceInterpolationMap;
  internal::FaceInterpolationOperatorKey
    key(numberOfDoFsPerAxisInPatch, overlap, normal);

  assertionEquals(overlap, 1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (faceInterpolationMap.count(key) == 0) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(
      numberOfDoFsPerAxisInPatch,
      normal,
      false,
      true
    );
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * 2 * overlap;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * 2 * overlap;
#endif

  logDebug(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString() << ": " << P->toString()
  );

  P->batchedMultiplyAoS(
    fineGridFaceValues,   // image
    coarseGridFaceValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  if (marker.isInteriorFaceWithinPatch()) {
#if Dimensions == 2
    const int patchSize = numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#else
    const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#endif

    double* leftAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );

    tarch::la::Vector<Dimensions, int>
      leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions, int>
      rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex(marker.getSelectedFaceNumber() % Dimensions)--;

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
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      leftAdjacentPatchData,
      rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData, tarch::MemoryLocation::Heap);
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap);
  } else {
    interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::CellMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    double* __restrict__ fineGridCellValues
  ) {
  logTraceInWith3Arguments(
    "interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    unknowns
  );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    false
  );

  logTraceOut(
    "interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  static internal::FaceInterpolationMap faceInterpolationMap;
  internal::FaceInterpolationOperatorKey
    key(numberOfDoFsPerAxisInPatch, overlap, normal);

  assertionEquals(overlap, 1);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (faceInterpolationMap.count(key) == 0) {
    faceInterpolationMap[key] = internal::createLinearInterpolationMatrix(
      numberOfDoFsPerAxisInPatch,
      normal,
      true,
      true
    );
  }
  tarch::la::DynamicMatrix* P = faceInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * 2 * overlap;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * 2 * overlap;
#endif

  logDebug(
    "interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString() << ": " << P->toString()
  );

  P->batchedMultiplyAoS(
    fineGridFaceValues,   // image
    coarseGridFaceValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialisePatchIndexInOverlap(
      marker.getRelativePositionWithinFatherCell(),
      normal
    ) * patchSize
  );

  logTraceOut(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)"
  );
}

void toolbox::blockstructured::
  interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    const double* __restrict__ coarseGridFaceValues,
    double* __restrict__ fineGridFaceValues
  ) {
  logTraceInWith4Arguments(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap,
    unknowns
  );

  if (marker.isInteriorFaceWithinPatch()) {
#if Dimensions == 2
    const int patchSize = numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#else
    const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                          * numberOfDoFsPerAxisInPatch;
#endif

    double* leftAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );
    double* rightAdjacentPatchData = tarch::allocateMemory<double>(
      patchSize * unknowns,
      tarch::MemoryLocation::Heap
    );

    tarch::la::Vector<Dimensions, int>
      leftAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    tarch::la::Vector<Dimensions, int>
      rightAdjacentPatchIndex = marker.getRelativePositionWithinFatherCell();
    leftAdjacentPatchIndex(marker.getSelectedFaceNumber() % Dimensions)--;

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
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      leftAdjacentPatchData,
      rightAdjacentPatchData,
      fineGridFaceValues
    );

    tarch::freeMemory(leftAdjacentPatchData, tarch::MemoryLocation::Heap);
    tarch::freeMemory(rightAdjacentPatchData, tarch::MemoryLocation::Heap);
  } else {
    interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      coarseGridFaceValues,
      fineGridFaceValues
    );
  }

  logTraceOut(
    "interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)"
  );
}


void toolbox::blockstructured::
  interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
    int numberOfDoFsPerAxisInSourcePatch,
    int numberOfDoFsPerAxisInDestinationPatch,
    int haloSourcePatch,
    int haloDestinationPatch,
    int unknowns,
    const double* __restrict__ sourceValues,
    double* __restrict__ destinationValues,
    ::peano4::utils::LoopPlacement parallelisation
  ) {
  assertion(
    numberOfDoFsPerAxisInDestinationPatch > numberOfDoFsPerAxisInSourcePatch
  );

  simtDforWithSchedulerInstructions(
    destinationVolume,
    numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch,
    parallelisation
  ) const int baseIndexDestination
    = peano4::utils::dLinearised(
        destinationVolume,
        numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch
      )
      * unknowns;
  for (int unknown = 0; unknown < unknowns; unknown++) {
    destinationValues[baseIndexDestination + unknown] = 0.0;
  }

  dfor(sourceVolume, numberOfDoFsPerAxisInSourcePatch + 2 * haloSourcePatch) {
    tarch::la::Vector<Dimensions, double>
      normalisedSourcePosition = tarch::la::multiplyComponents(
        tarch::la::
          Vector<Dimensions, double>(1.0 / numberOfDoFsPerAxisInSourcePatch),
        tarch::la::convertScalar<double>(sourceVolume
        ) + tarch::la::Vector<Dimensions, double>(0.5 - haloSourcePatch)
      );
    tarch::la::Vector<Dimensions, double>
      normalisedDestinationPosition = tarch::la::multiplyComponents(
        tarch::la::Vector<Dimensions, double>(
          1.0 / numberOfDoFsPerAxisInDestinationPatch
        ),
        tarch::la::convertScalar<double>(destinationVolume
        ) + tarch::la::Vector<Dimensions, double>(0.5 - haloDestinationPatch)
      );

    int outsidePatchAlongCoordinateAxis = 0;
    for (int d = 0; d < Dimensions; d++) {
      //        outsidePatchAlongCoordinateAxis +=  ( sourceVolume(d) <
      //        haloSourcePatch ) ? 1 : 0; outsidePatchAlongCoordinateAxis +=  (
      //        sourceVolume(d) >=
      //        haloSourcePatch+numberOfDoFsPerAxisInSourcePatch ) ? 1 : 0;
      if (sourceVolume(d) < haloSourcePatch)
        outsidePatchAlongCoordinateAxis++;
      if (sourceVolume(d) >= haloSourcePatch + numberOfDoFsPerAxisInSourcePatch)
        outsidePatchAlongCoordinateAxis++;
    }
    const bool isFromUnitialisedDiagonal = outsidePatchAlongCoordinateAxis > 1;

    double weight = 1.0;
    for (int d = 0; d < Dimensions; d++) {
      double distanceAlongCoordinateAxis = std::abs(
        normalisedDestinationPosition(d) - normalisedSourcePosition(d)
      );
      double h = 1.0 / static_cast<double>(numberOfDoFsPerAxisInSourcePatch);
      distanceAlongCoordinateAxis = 1.0 - distanceAlongCoordinateAxis / h;
      weight *= std::max(0.0, distanceAlongCoordinateAxis);
    }
#if !defined(SharedSYCL)
    assertion(tarch::la::greaterEquals(weight, 0.0));
    assertion(tarch::la::smallerEquals(weight, 1.0));
#endif

    tarch::la::Vector<Dimensions, int> amendedSourceVolume = sourceVolume;
    if (isFromUnitialisedDiagonal) {
#pragma unroll
      for (int d = 0; d < Dimensions; d++) {
        amendedSourceVolume(d
        ) = std::max(amendedSourceVolume(d), haloSourcePatch);
        amendedSourceVolume(d) = std::min(
          amendedSourceVolume(d),
          haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1
        );
      }
    }

    int baseIndexSource = peano4::utils::dLinearised(
                            amendedSourceVolume,
                            numberOfDoFsPerAxisInSourcePatch + 2 * haloSourcePatch
                          )
                          * unknowns;
    for (int unknown = 0; unknown < unknowns; unknown++) {
      destinationValues[baseIndexDestination + unknown]
        += weight * sourceValues[baseIndexSource + unknown];
    }
  }
  endSimtDfor
}


void toolbox::blockstructured::
  interpolateCellDataAssociatedToVolumesIntoOverlappingCell_matrix(
    int numberOfDoFsPerAxisInSourcePatch,
    int numberOfDoFsPerAxisInDestinationPatch,
    int haloSourcePatch,
    int haloDestinationPatch,
    int unknowns,
    const double* __restrict__ interpolationData,
    const int* __restrict__ columnIndices,
    const int* __restrict__ rowIndices,
    const double* __restrict__ sourceValues,
    double* __restrict__ destinationValues,
    ::peano4::utils::LoopPlacement parallelisation
  ) {
  int destinationSize
    = (numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch)
      * (numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch)
      * (numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch);

  for (int row = 0; row < destinationSize; row++) {
    for (int unknown = 0; unknown < unknowns; unknown++) {
      destinationValues[row * unknowns + unknown] = 0.0;
    }
    for (int column = rowIndices[row]; column < rowIndices[row + 1]; column++) {
      for (int unknown = 0; unknown < unknowns; unknown++) {
        destinationValues[row * unknowns + unknown]
          += interpolationData[column]
             * sourceValues[columnIndices[column] * unknowns + unknown];
      }
    }
  }
}

void toolbox::blockstructured::
  interpolateCellDataAssociatedToVolumesIntoOverlappingCell_secondOrder(
    int numberOfDoFsPerAxisInSourcePatch,
    int numberOfDoFsPerAxisInDestinationPatch,
    int haloSourcePatch,
    int haloDestinationPatch,
    int unknowns,
    const double* __restrict__ sourceValues,
    double* __restrict__ destinationValues,
    ::peano4::utils::LoopPlacement parallelisation
  ) {

  int                                indices[18];
  tarch::la::Vector<Dimensions, int> stencil[18];
  double*                            centerValues = new double[unknowns];
  int numberOfDerivatives                         = (Dimensions == 2) ? 5 : 9;

  dfor(volume, numberOfDoFsPerAxisInSourcePatch + 2) {
    tarch::la::Vector<Dimensions, int>
      sourceVolume = volume
                     + tarch::la::Vector<Dimensions, int>(haloSourcePatch - 1);
    int outsidePatchAlongCoordinateAxis = 0;
    for (int d = 0; d < Dimensions; d++) {
      if (sourceVolume(d) < haloSourcePatch || sourceVolume(d) >= numberOfDoFsPerAxisInSourcePatch + haloSourcePatch) {
        outsidePatchAlongCoordinateAxis++;
      }
    }

    tarch::la::Vector<Dimensions, int> stencilOffset(0);
    for (int d = 0; d < Dimensions; d++) {
      if (sourceVolume(d) == 0)
        stencilOffset(d) = 1;
      if (sourceVolume(d) == numberOfDoFsPerAxisInSourcePatch + 2 * haloSourcePatch - 1)
        stencilOffset = -1;
    }
    for (int d1 = 0; d1 < Dimensions; d1++) {
      for (int d2 = d1 + 1; d2 < Dimensions; d2++) {
        if (sourceVolume(d1) == haloSourcePatch and sourceVolume(d2) == haloSourcePatch) {
          stencilOffset(d1) = 1;
          stencilOffset(d2) = 1;
        }
        if (sourceVolume(d1) == haloSourcePatch and sourceVolume(d2) == haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1) {
          stencilOffset(d1) = 1;
          stencilOffset(d2) = -1;
        }
        if (sourceVolume(d1) == haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1 and sourceVolume(d2) == haloSourcePatch) {
          stencilOffset(d1) = -1;
          stencilOffset(d2) = 1;
        }
        if (sourceVolume(d1) == haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1 and sourceVolume(d2) == haloSourcePatch + numberOfDoFsPerAxisInSourcePatch - 1) {
          stencilOffset(d1) = -1;
          stencilOffset(d2) = -1;
        }
      }
    }

    if (outsidePatchAlongCoordinateAxis <= 1) {
      int stencilSize = 0;
      dfor(offset, 3) {
        if (
          offset != tarch::la::Vector<Dimensions, int>(1) - stencilOffset and
#if Dimensions == 3
          !(offset(0) + stencilOffset(0) != 1
            and offset(1) + stencilOffset(1) != 1
            and offset(2) + stencilOffset(2) != 1)
#elif Dimensions == 2
          !(offset(0) + stencilOffset(0) != 1
            and offset(1) + stencilOffset(1) != 1)
#endif
        ) {
          stencil[stencilSize] = stencilOffset + offset
                                 - tarch::la::Vector<Dimensions, int>(1);
          indices[stencilSize] = peano4::utils::dLinearised(
            sourceVolume + offset + stencilOffset
              - tarch::la::Vector<Dimensions, int>(1),
            numberOfDoFsPerAxisInSourcePatch + 2 * haloSourcePatch
          );
          stencilSize++;
        }
      }

      int centerIndex = peano4::utils::dLinearised(
        sourceVolume,
        numberOfDoFsPerAxisInSourcePatch + 2 * haloSourcePatch
      );
      for (int unknown = 0; unknown < unknowns; unknown++) {
        centerValues[unknown] = sourceValues[centerIndex * unknowns + unknown];
      }

      tarch::la::DynamicMatrix deltas(stencilSize, unknowns);
      for (int k = 0; k < stencilSize; k++) {
        for (int unknown = 0; unknown < unknowns; unknown++) {
          deltas(
            k,
            unknown
          ) = sourceValues[indices[k] * unknowns + unknown]
              - centerValues[unknown];
        }
      }
      tarch::la::DynamicMatrix A(stencilSize, numberOfDerivatives);
      for (int row = 0; row < stencilSize; row++) {
        int                                 column = 0;
        tarch::la::Vector<Dimensions, int>& cell   = stencil[row];
        for (int a = 0; a < Dimensions; a++) {
          A(row, column) = cell(a);
          column++;
        }
        for (int a = 0; a < Dimensions; a++) {
          for (int b = a; b < Dimensions; b++) {
            A(row, column) = 0.5 * cell(a) * cell(b);
            column++;
          }
        }
      }

      tarch::la::DynamicMatrix x(numberOfDerivatives, unknowns);
      if (A.rows() >= A.cols()){
        tarch::la::DynamicMatrix Q(A.rows(), A.cols());
        tarch::la::DynamicMatrix R(A.cols(), A.cols());

        tarch::la::modifiedGramSchmidt(A, Q, R);

        auto                     Q_T = tarch::la::transpose(Q);
        tarch::la::DynamicMatrix c   = Q_T * deltas;

        tarch::la::DynamicMatrix inverseR = tarch::la::invertUpperTriangular(R);
        inverseR.multiplyBySmallMatrix(x.data(), c);
      } else{
        tarch::la::DynamicMatrix Q(A.cols(), A.rows());
        tarch::la::DynamicMatrix R(A.rows(), A.rows());

        tarch::la::DynamicMatrix ATranspose = tarch::la::transpose(A);
        tarch::la::modifiedGramSchmidt(ATranspose, Q, R);

        tarch::la::DynamicMatrix inverseTransposeR = tarch::la::transpose(tarch::la::invertUpperTriangular(R));
        tarch::la::DynamicMatrix c   = inverseTransposeR * deltas;

        Q.multiplyBySmallMatrix(x.data(), c);
      }

      tarch::la::Vector<Dimensions, int> destinationCorner1
        = (sourceVolume - tarch::la::Vector<Dimensions, int>(haloSourcePatch)
          ) * numberOfDoFsPerAxisInDestinationPatch
            / numberOfDoFsPerAxisInSourcePatch
          + tarch::la::Vector<Dimensions, int>(haloDestinationPatch);
      tarch::la::Vector<Dimensions, int> destinationCorner2
        = (sourceVolume
           - tarch::la::Vector<Dimensions, int>(haloSourcePatch - 1)
          ) * numberOfDoFsPerAxisInDestinationPatch
            / numberOfDoFsPerAxisInSourcePatch
          + tarch::la::Vector<Dimensions, int>(haloDestinationPatch);
      for (int d = 0; d < Dimensions; d++) {
        destinationCorner1(d) = std::max(0, destinationCorner1(d));
        destinationCorner2(d) = std::min(
          numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch,
          destinationCorner2(d)
        );
      }

      tarch::la::Vector<Dimensions, int>
          destinationSpan = destinationCorner2 - destinationCorner1;
      int destinationDimensionsProduct = 1;
      for (int d = 0; d < Dimensions; d++)
        destinationDimensionsProduct *= destinationSpan(d);

      int                                   row = 0;
      tarch::la::Vector<Dimensions, double> normalisedCornerPosition
        = (1.0 / numberOfDoFsPerAxisInDestinationPatch)
          * (tarch::la::convertScalar<double>(destinationCorner1) + tarch::la::Vector<Dimensions, double>(0.5 - haloDestinationPatch));
      tarch::la::Vector<Dimensions, double> normalisedSourcePosition
        = (1.0 / numberOfDoFsPerAxisInSourcePatch)
          * (tarch::la::convertScalar<double>(sourceVolume) + tarch::la::Vector<Dimensions, double>(0.5 - haloSourcePatch));
      tarch::la::Vector<Dimensions, double>
        cornerDisplacement = (normalisedCornerPosition
                              - normalisedSourcePosition)
                             * (double)numberOfDoFsPerAxisInSourcePatch;

      double* matrixData = new double
        [destinationDimensionsProduct * numberOfDerivatives];
      int writeIndex = 0;

      dfor(localDestinationVolume, destinationSpan) {
        tarch::la::Vector<Dimensions, double>
          displacement = cornerDisplacement
                         + ((double)numberOfDoFsPerAxisInSourcePatch
                            / numberOfDoFsPerAxisInDestinationPatch)
                             * tarch::la::convertScalar<
                               double>(localDestinationVolume);

        for (int a = 0; a < Dimensions; a++) {
          matrixData[writeIndex] = displacement(a);
          writeIndex++;
        }
        for (int a = 0; a < Dimensions; a++) {
          for (int b = a; b < Dimensions; b++) {
            matrixData[writeIndex] = 0.5 * displacement(a) * displacement(b);
            writeIndex++;
          }
        }
      }

      tarch::la::DynamicMatrix destinationMatrix(
        destinationDimensionsProduct,
        numberOfDerivatives,
        matrixData
      );
      tarch::la::DynamicMatrix destinationValuesDelta = destinationMatrix * x;

      int destinationVolumeNumber = 0;
      dfor(localDestinationVolume, destinationSpan) {
        tarch::la::Vector<Dimensions, int>
            destinationVolume = destinationCorner1 + localDestinationVolume;
        int destinationIndex  = peano4::utils::dLinearised(
          destinationVolume,
          numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch
        );
        for (int unknown = 0; unknown < unknowns; unknown++) {
          destinationValues[destinationIndex * unknowns + unknown]
            = centerValues[unknown]
              + destinationValuesDelta(destinationVolumeNumber, unknown);
        }
        destinationVolumeNumber++;
      }
    }
  }
  delete[] centerValues;
}

void toolbox::blockstructured::
  interpolateCellDataAssociatedToVolumesIntoOverlappingCell_fourthOrder(
    int numberOfDoFsPerAxisInSourcePatch,
    int numberOfDoFsPerAxisInDestinationPatch,
    int haloSourcePatch,
    int haloDestinationPatch,
    int unknowns,
    const double* __restrict__ sourceValues,
    double* __restrict__ destinationValues,
    ::peano4::utils::LoopPlacement parallelisation
  ) {
  assertion(
    numberOfDoFsPerAxisInDestinationPatch > numberOfDoFsPerAxisInSourcePatch
  );

  simtDforWithSchedulerInstructions(
    destinationVolume,
    numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch,
    parallelisation
  ) const int baseIndexDestination
    = peano4::utils::dLinearised(
        destinationVolume,
        numberOfDoFsPerAxisInDestinationPatch + 2 * haloDestinationPatch
      )
      * unknowns;
  for (int unknown = 0; unknown < unknowns; unknown++) {
    destinationValues[baseIndexDestination + unknown] = 0.0;
  }
  endSimtDfor
}


void toolbox::blockstructured::
  interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
    const peano4::datamanagement::CellMarker& marker,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       unknowns,
    const double* __restrict__ coarseGridCellValues,
    double* __restrict__ fineGridCellValues
  ) {
  logTraceInWith3Arguments(
    "interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    unknowns
  );

  internal::interpolateCell_AoS_linear(
    marker.getRelativePositionWithinFatherCell(),
    numberOfDoFsPerAxisInPatch,
    unknowns,
    coarseGridCellValues,
    fineGridCellValues,
    true
  );

  logTraceOut(
    "interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(...)"
  );
}

void toolbox::blockstructured::internal::interpolateCell_AoS_linear(
  const tarch::la::Vector<Dimensions, int>& relativePositionWithinFatherCell,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  const double* __restrict__ coarseGridCellValues,
  double* __restrict__ fineGridCellValues,
  bool extrapolateLinearly
) {
  static internal::CellInterpolationMap  cellInterpolationMap;
  internal::CellInterpolationOperatorKey key(numberOfDoFsPerAxisInPatch);

  tarch::multicore::Lock lock(interpolationMapSemaphore);
  if (cellInterpolationMap.count(key) == 0) {
    cellInterpolationMap[key] = internal::createLinearInterpolationMatrix(
      numberOfDoFsPerAxisInPatch,
      extrapolateLinearly
    );
  }
  tarch::la::DynamicMatrix* P = cellInterpolationMap[key];
  lock.free();

#if Dimensions == 2
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch;
#else
  const int patchSize = numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch
                        * numberOfDoFsPerAxisInPatch;
#endif

  P->batchedMultiplyAoS(
    fineGridCellValues,   // image
    coarseGridCellValues, // preimage
    unknowns,  // batch size, i.e. how often to apply it in one AoS rush
    patchSize, // result size, i.e. size of image
    serialiseMarkerIn3x3PatchAssembly(
      relativePositionWithinFatherCell,
      numberOfDoFsPerAxisInPatch
    )
  );
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_second_order(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                coarseGridFaceValues,
  double* __restrict__                      fineGridFaceValues
) {
  const int normal = marker.getSelectedFaceNumber() % Dimensions;
  const int i      = (normal + 1) % Dimensions;
#if Dimensions == 3
  const int j = (normal + 2) % Dimensions;
#endif

  tarch::la::Vector<Dimensions, int> coarseStart(0);
  coarseStart(i
  ) = marker.getRelativePositionWithinFatherCell()(i)
      * numberOfDoFsPerAxisInPatch / 3;
#if Dimensions == 3
  coarseStart(j
  ) = marker.getRelativePositionWithinFatherCell()(j)
      * numberOfDoFsPerAxisInPatch / 3;
#endif

  tarch::la::Vector<Dimensions, int> coarseEnd(0);
  coarseEnd(i
  ) = ((marker.getRelativePositionWithinFatherCell()(i) + 1
       ) * numberOfDoFsPerAxisInPatch
       + 2)
      / 3;
#if Dimensions == 3
  coarseEnd(j
  ) = ((marker.getRelativePositionWithinFatherCell()(j) + 1
       ) * numberOfDoFsPerAxisInPatch
       + 2)
      / 3;
#endif

  tarch::la::Vector<Dimensions, int> fineDataStart(0);
  fineDataStart(i
  ) = (marker.getRelativePositionWithinFatherCell()(i)
       * numberOfDoFsPerAxisInPatch)
      % 3;
#if Dimensions == 3
  fineDataStart(j
  ) = (marker.getRelativePositionWithinFatherCell()(j)
       * numberOfDoFsPerAxisInPatch)
      % 3;
#endif

  tarch::la::Vector<Dimensions, int> faceDimensions = coarseEnd - coarseStart;
  faceDimensions(normal)                            = (overlap - 1) / 3 + 1;

  int                                indices[18];
  tarch::la::Vector<Dimensions, int> stencil[18];
  double*                            centerValues = new double[unknowns];

  dfor(kCoarse, faceDimensions) {
    tarch::la::Vector<Dimensions, int> center = kCoarse + coarseStart;
    center(normal
    ) = (marker.getSelectedFaceNumber() < Dimensions)
          ? overlap - kCoarse(normal) - 1
          : overlap + kCoarse(normal);
    tarch::la::Vector<Dimensions, int> stencilOffset(0);

    if (center(i) == 0)
      stencilOffset(i) = 1;
    if (center(i) == numberOfDoFsPerAxisInPatch - 1)
      stencilOffset(i) = -1;
#if Dimensions == 3
    if (center(j) == 0)
      stencilOffset(j) = 1;
    if (center(j) == numberOfDoFsPerAxisInPatch - 1)
      stencilOffset(j) = -1;
#endif

    int stencilSize = 0;
    dfor(offset, 3) {
      if (
        offset != tarch::la::Vector<Dimensions, int>(1) - stencilOffset and
#if Dimensions == 3
        !(offset(0) + stencilOffset(0) != 1
          and offset(1) + stencilOffset(1) != 1
          and offset(2) + stencilOffset(2) != 1)
#elif Dimensions == 2
        !(offset(0) + stencilOffset(0) != 1
          and offset(1) + stencilOffset(1) != 1)
#endif
      ) {
        stencil[stencilSize] = stencilOffset + offset
                               - tarch::la::Vector<Dimensions, int>(1);
        indices[stencilSize] = serialiseVoxelIndexInOverlap(
          center + offset + stencilOffset
            - tarch::la::Vector<Dimensions, int>(1),
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        stencilSize++;
      }
    }

    int index = serialiseVoxelIndexInOverlap(
      center,
      numberOfDoFsPerAxisInPatch,
      overlap,
      normal
    );
    for (int unknown = 0; unknown < unknowns; unknown++) {
      centerValues[unknown] = coarseGridFaceValues[index * unknowns + unknown];
    }

    tarch::la::DynamicMatrix deltas(stencilSize, unknowns);
    for (int k = 0; k < stencilSize; k++) {
      for (int unknown = 0; unknown < unknowns; unknown++) {
        deltas(
          k,
          unknown
        ) = coarseGridFaceValues[indices[k] * unknowns + unknown]
            - centerValues[unknown];
      }
    }

    int                      numberOfDerivatives = (Dimensions == 2) ? 5 : 9;
    tarch::la::DynamicMatrix A(stencilSize, numberOfDerivatives);
    for (int row = 0; row < stencilSize; row++) {
      int                                 column = 0;
      tarch::la::Vector<Dimensions, int>& cell   = stencil[row];
      for (int a = 0; a < Dimensions; a++) {
        A(row, column) = cell(a);
        column++;
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          A(row, column) = 0.5 * cell(a) * cell(b);
          column++;
        }
      }
    }

    tarch::la::DynamicMatrix x(numberOfDerivatives, unknowns);
    if (A.rows() >= A.cols()){
      tarch::la::DynamicMatrix Q(A.rows(), A.cols());
      tarch::la::DynamicMatrix R(A.cols(), A.cols());

      tarch::la::modifiedGramSchmidt(A, Q, R);

      auto                     Q_T = tarch::la::transpose(Q);
      tarch::la::DynamicMatrix c   = Q_T * deltas;

      tarch::la::DynamicMatrix inverseR = tarch::la::invertUpperTriangular(R);
      inverseR.multiplyBySmallMatrix(x.data(), c);
    } else{
      tarch::la::DynamicMatrix Q(A.cols(), A.rows());
      tarch::la::DynamicMatrix R(A.rows(), A.rows());

      tarch::la::DynamicMatrix ATranspose = tarch::la::transpose(A);
      tarch::la::modifiedGramSchmidt(ATranspose, Q, R);

      tarch::la::DynamicMatrix inverseTransposeR = tarch::la::transpose(tarch::la::invertUpperTriangular(R));
      tarch::la::DynamicMatrix c   = inverseTransposeR * deltas;

      Q.multiplyBySmallMatrix(x.data(), c);
    }

    tarch::la::Vector<Dimensions, int> fineStart(0);
    tarch::la::Vector<Dimensions, int> fineEnd(3);

    if (kCoarse(i) == 0)
      fineStart(i
      ) = (marker.getRelativePositionWithinFatherCell()(i)
           * numberOfDoFsPerAxisInPatch)
          % 3;
#if Dimensions == 3
    if (kCoarse(j) == 0)
      fineStart(j
      ) = (marker.getRelativePositionWithinFatherCell()(j)
           * numberOfDoFsPerAxisInPatch)
          % 3;
#endif

    int modulus = (marker.getRelativePositionWithinFatherCell()(i) + 1)
                  * numberOfDoFsPerAxisInPatch;
    if (modulus % 3 != 0)
      modulus = modulus % 3;
    else
      modulus = 3;
    if (kCoarse(i) == faceDimensions(i) - 1)
      fineEnd(i) = modulus;

#if Dimensions == 3
    modulus = (marker.getRelativePositionWithinFatherCell()(j) + 1)
              * numberOfDoFsPerAxisInPatch;
    if (modulus % 3 != 0)
      modulus = modulus % 3;
    else
      modulus = 3;
    if (kCoarse(j) == faceDimensions(j) - 1)
      fineEnd(j) = modulus;
#endif

    tarch::la::Vector<Dimensions, int> fineDimensions = fineEnd - fineStart;
    fineDimensions(normal) = std::min(overlap - kCoarse(normal) * 3, 3);
    if (fineDimensions(normal) % 3 != 0)
      fineDimensions(normal) = fineDimensions(normal) % 3;
    else
      fineDimensions(normal) = 3;
#if Dimensions == 3
    int fineDimensionsProduct = fineDimensions(normal) * fineDimensions(i)
                                * fineDimensions(j);
#elif Dimensions == 2
    int fineDimensionsProduct = fineDimensions(normal) * fineDimensions(i);
#endif
    tarch::la::DynamicMatrix
        fineMatrix(fineDimensionsProduct, numberOfDerivatives);
    int row = 0;

    dfor(kFine, fineDimensions) {
      tarch::la::Vector<Dimensions, int>
          cell   = fineStart + kFine - tarch::la::Vector<Dimensions, int>(1);
      int column = 0;
      for (int a = 0; a < Dimensions; a++) {
        fineMatrix(row, column) = (1.0 / 3.0) * cell(a);
        column++;
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          fineMatrix(row, column) = (1.0 / 18.0) * cell(a) * cell(b);
          column++;
        }
      }
      row++;
    }
    tarch::la::DynamicMatrix fineValues = fineMatrix * x;

    int fineCellNumber = 0;
    tarch::la::Vector<Dimensions, int>
      fineCorner = kCoarse * 3 + fineStart - fineDataStart;
    fineCorner(normal
    )            = (marker.getSelectedFaceNumber() < Dimensions)
                     ? (kCoarse(normal) * 3)
                     : overlap + (kCoarse(normal) * 3);

    dfor(kFine, fineDimensions) {
      tarch::la::Vector<Dimensions, int> fineCell = kFine + fineCorner;
      int                                index = serialiseVoxelIndexInOverlap(
        fineCell,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );

      for (int unknown = 0; unknown < unknowns; unknown++) {
        fineGridFaceValues[index * unknowns + unknown]
          = centerValues[unknown] + fineValues(fineCellNumber, unknown);
      }
      fineCellNumber++;
    }
  }

  delete[] centerValues;

}

void toolbox::blockstructured::interpolateHaloLayer_AoS_third_order(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__ coarseGridFaceValues,
  double* __restrict__ fineGridFaceValues
) {
  const int normal = marker.getSelectedFaceNumber() % Dimensions;
  const int i      = (normal + 1) % Dimensions;
#if Dimensions == 3
  const int j = (normal + 2) % Dimensions;
#endif

  tarch::la::Vector<Dimensions, int> coarseStart(0);
  coarseStart(i
  ) = marker.getRelativePositionWithinFatherCell()(i)
      * numberOfDoFsPerAxisInPatch / 3;
#if Dimensions == 3
  coarseStart(j
  ) = marker.getRelativePositionWithinFatherCell()(j)
      * numberOfDoFsPerAxisInPatch / 3;
#endif

  tarch::la::Vector<Dimensions, int> coarseEnd(0);
  coarseEnd(i
  ) = ((marker.getRelativePositionWithinFatherCell()(i) + 1
       ) * numberOfDoFsPerAxisInPatch
       + 2)
      / 3;
#if Dimensions == 3
  coarseEnd(j
  ) = ((marker.getRelativePositionWithinFatherCell()(j) + 1
       ) * numberOfDoFsPerAxisInPatch
       + 2)
      / 3;
#endif

  tarch::la::Vector<Dimensions, int> fineDataStart(0);
  fineDataStart(i
  ) = (marker.getRelativePositionWithinFatherCell()(i)
       * numberOfDoFsPerAxisInPatch)
      % 3;
#if Dimensions == 3
  fineDataStart(j
  ) = (marker.getRelativePositionWithinFatherCell()(j)
       * numberOfDoFsPerAxisInPatch)
      % 3;
#endif

  tarch::la::Vector<Dimensions, int> faceDimensions = coarseEnd - coarseStart;
  faceDimensions(normal)                            = (overlap - 1) / 3 + 1;

  double*                            centerValues = new double[unknowns];
  int                                indices[32];
  tarch::la::Vector<Dimensions, int> stencil[32];


  dfor(kCoarse, faceDimensions) {
    tarch::la::Vector<Dimensions, int> center = kCoarse + coarseStart;
    center(normal
    ) = (marker.getSelectedFaceNumber() < Dimensions)
          ? overlap - kCoarse(normal) - 1
          : overlap + kCoarse(normal);
    tarch::la::Vector<Dimensions, int> stencilOffset(0);

    if (center(i) == 0)
      stencilOffset(i) = 1;
    if (center(i) == numberOfDoFsPerAxisInPatch - 1)
      stencilOffset(i) = -1;
#if Dimensions == 3
    if (center(j) == 0)
      stencilOffset(j) = 1;
    if (center(j) == numberOfDoFsPerAxisInPatch - 1)
      stencilOffset(j) = -1;
#endif

    int stencilSize = 0;
    dfor(offset, 3) {
      if (offset != tarch::la::Vector<Dimensions, int>(1) - stencilOffset) {
        stencil[stencilSize] = stencilOffset + offset
                               - tarch::la::Vector<Dimensions, int>(1);
        indices[stencilSize] = serialiseVoxelIndexInOverlap(
          center + offset + stencilOffset
            - tarch::la::Vector<Dimensions, int>(1),
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        stencilSize++;
      }
    }

    for (int axis = 0; axis < Dimensions; axis++) {
      if (center(axis) > 1) {
        tarch::la::Vector<Dimensions, int> offset(0);
        offset(axis)         = -2;
        stencil[stencilSize] = stencilOffset + offset;
        indices[stencilSize] = serialiseVoxelIndexInOverlap(
          center + stencilOffset + offset,
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        stencilSize++;
      }
      if (center(axis) < numberOfDoFsPerAxisInPatch - 2) {
        tarch::la::Vector<Dimensions, int> offset(0);
        offset(axis)         = 2;
        stencil[stencilSize] = stencilOffset + offset;
        indices[stencilSize] = serialiseVoxelIndexInOverlap(
          center + stencilOffset + offset,
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        stencilSize++;
      }
    }

    int index = serialiseVoxelIndexInOverlap(
      center,
      numberOfDoFsPerAxisInPatch,
      overlap,
      normal
    );
    for (int unknown = 0; unknown < unknowns; unknown++) {
      centerValues[unknown] = coarseGridFaceValues[index * unknowns + unknown];
    }
    tarch::la::DynamicMatrix deltas(stencilSize, unknowns);
    for (int k = 0; k < stencilSize; k++) {
      for (int unknown = 0; unknown < unknowns; unknown++) {
        deltas(
          k,
          unknown
        ) = coarseGridFaceValues[indices[k] * unknowns + unknown]
            - centerValues[unknown];
      }
    }

    int                      numberOfDerivatives = (Dimensions == 2) ? 9 : 19;
    tarch::la::DynamicMatrix A(stencilSize, numberOfDerivatives);

    for (int row = 0; row < stencilSize; row++) {
      int                                 column = 0;
      tarch::la::Vector<Dimensions, int>& cell   = stencil[row];
      for (int a = 0; a < Dimensions; a++) {
        A(row, column) = cell(a);
        column++;
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          A(row, column) = 0.5 * cell(a) * cell(b);
          column++;
        }
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          for (int c = b; c < Dimensions; c++) {
            A(row, column) = (1.0 / 6.0) * cell(a) * cell(b) * cell(c);
            column++;
          }
        }
      }
    }

    tarch::la::DynamicMatrix x(numberOfDerivatives, unknowns);
    if (A.rows() >= A.cols()){
      tarch::la::DynamicMatrix Q(A.rows(), A.cols());
      tarch::la::DynamicMatrix R(A.cols(), A.cols());

      tarch::la::modifiedGramSchmidt(A, Q, R);

      auto                     Q_T = tarch::la::transpose(Q);
      tarch::la::DynamicMatrix c   = Q_T * deltas;

      tarch::la::DynamicMatrix inverseR = tarch::la::invertUpperTriangular(R);
      inverseR.multiplyBySmallMatrix(x.data(), c);
    } else{
      tarch::la::DynamicMatrix Q(A.cols(), A.rows());
      tarch::la::DynamicMatrix R(A.rows(), A.rows());

      tarch::la::DynamicMatrix ATranspose = tarch::la::transpose(A);
      tarch::la::modifiedGramSchmidt(ATranspose, Q, R);

      tarch::la::DynamicMatrix inverseTransposeR = tarch::la::transpose(tarch::la::invertUpperTriangular(R));
      tarch::la::DynamicMatrix c   = inverseTransposeR * deltas;
      Q.multiplyBySmallMatrix(x.data(), c);
    }

    tarch::la::Vector<Dimensions, int> fineStart(0);
    tarch::la::Vector<Dimensions, int> fineEnd(3);

    if (kCoarse(i) == 0)
      fineStart(i
      ) = (marker.getRelativePositionWithinFatherCell()(i)
           * numberOfDoFsPerAxisInPatch)
          % 3;
#if Dimensions == 3
    if (kCoarse(j) == 0)
      fineStart(j
      ) = (marker.getRelativePositionWithinFatherCell()(j)
           * numberOfDoFsPerAxisInPatch)
          % 3;
#endif

    int modulus = (marker.getRelativePositionWithinFatherCell()(i) + 1)
                  * numberOfDoFsPerAxisInPatch;
    if (modulus % 3 != 0)
      modulus = modulus % 3;
    else
      modulus = 3;
    if (kCoarse(i) == faceDimensions(i) - 1)
      fineEnd(i) = modulus;

#if Dimensions == 3
    modulus = (marker.getRelativePositionWithinFatherCell()(j) + 1)
              * numberOfDoFsPerAxisInPatch;
    if (modulus % 3 != 0)
      modulus = modulus % 3;
    else
      modulus = 3;
    if (kCoarse(j) == faceDimensions(j) - 1)
      fineEnd(j) = modulus;
#endif

    tarch::la::Vector<Dimensions, int> fineDimensions = fineEnd - fineStart;
    fineDimensions(normal) = std::min(overlap - kCoarse(normal) * 3, 3);
    if (fineDimensions(normal) % 3 != 0)
      fineDimensions(normal) = fineDimensions(normal) % 3;
    else
      fineDimensions(normal) = 3;

#if Dimensions == 3
    int fineDimensionsProduct = fineDimensions(normal) * fineDimensions(i)
                                * fineDimensions(j);
#elif Dimensions == 2
    int fineDimensionsProduct = fineDimensions(normal) * fineDimensions(i);
#endif

    tarch::la::DynamicMatrix
        fineMatrix(fineDimensionsProduct, numberOfDerivatives);
    int row = 0;

    dfor(kFine, fineDimensions) {
      tarch::la::Vector<Dimensions, int>
          cell   = fineStart + kFine - tarch::la::Vector<Dimensions, int>(1);
      int column = 0;
      for (int a = 0; a < Dimensions; a++) {
        fineMatrix(row, column) = (1.0 / 3.0) * cell(a);
        column++;
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          fineMatrix(row, column) = (1.0 / 18.0) * cell(a) * cell(b);
          column++;
        }
      }
      for (int a = 0; a < Dimensions; a++) {
        for (int b = a; b < Dimensions; b++) {
          for (int c = b; c < Dimensions; c++) {
            fineMatrix(
              row,
              column
            ) = (1.0 / 162.0) * cell(a) * cell(b) * cell(c);
            column++;
          }
        }
      }
      row++;
    }
    tarch::la::DynamicMatrix fineValues = fineMatrix * x;

    int fineCellNumber = 0;
    tarch::la::Vector<Dimensions, int>
      fineCorner = kCoarse * 3 + fineStart - fineDataStart;
    fineCorner(normal
    )            = (marker.getSelectedFaceNumber() < Dimensions)
                     ? (kCoarse(normal) * 3)
                     : overlap + (kCoarse(normal) * 3);

    dfor(kFine, fineDimensions) {
      tarch::la::Vector<Dimensions, int> fineCell = kFine + fineCorner;
      int                                index = serialiseVoxelIndexInOverlap(
        fineCell,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );

      for (int unknown = 0; unknown < unknowns; unknown++) {
        fineGridFaceValues[index * unknowns + unknown]
          = centerValues[unknown] + fineValues(fineCellNumber, unknown);
      }
      fineCellNumber++;
    }
  }

  delete[] centerValues;
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_matrix(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__ interpolationData,
  const int* __restrict__ columnIndices,
  const int* __restrict__ rowIndices,
  int dataStart,
  int rowIndicesStart,
  const double* __restrict__ coarseGridFaceValues,
  double* __restrict__ fineGridFaceValues
) {
  tarch::la::Vector<Dimensions, int> fineFaceDimensions(
    numberOfDoFsPerAxisInPatch
  );
  tarch::la::Vector<Dimensions, int> coarseFaceDimensions(
    numberOfDoFsPerAxisInPatch
  );

  const int normal        = marker.getSelectedFaceNumber() % Dimensions;
  const int matrixColumns = 2 * overlap
                            * std::
                              pow(numberOfDoFsPerAxisInPatch, Dimensions - 1);
  const int matrixRows = overlap
                         * std::pow(numberOfDoFsPerAxisInPatch, Dimensions - 1);

  fineFaceDimensions(normal)   = overlap;
  coarseFaceDimensions(normal) = 2 * overlap;

  double* rearrangedCoarseData = new double[matrixColumns * unknowns];

  dfor(kCoarse, coarseFaceDimensions) {
    int currentIndex = serialiseVoxelIndexInOverlap(
      kCoarse,
      numberOfDoFsPerAxisInPatch,
      overlap,
      normal
    );
    int newIndex = (marker.getSelectedFaceNumber() >= Dimensions)
                     ? kCoarse(normal)
                     : 2 * overlap - kCoarse(normal) - 1;
    int base     = overlap * 2;
    for (int d = 1; d < Dimensions; d++) {
      newIndex += kCoarse((normal + d) % Dimensions) * base;
      base *= numberOfDoFsPerAxisInPatch;
    }
    for (int unknown = 0; unknown < unknowns; unknown++) {
      rearrangedCoarseData[newIndex * unknowns + unknown] = coarseGridFaceValues
        [currentIndex * unknowns + unknown];
    }
  }

  dfor(kFine, fineFaceDimensions) {
    auto fineCoords = kFine;
    fineCoords(normal)  = (marker.getSelectedFaceNumber() < Dimensions)
                        ? overlap - kFine(normal) - 1
                        : overlap + kFine(normal);

    int fineIndex = serialiseVoxelIndexInOverlap(
      fineCoords,
      numberOfDoFsPerAxisInPatch,
      overlap,
      normal
    );

    int row  = kFine(normal);
    int base = overlap;
    for (int d = 1; d < Dimensions; d++) {
      row += kFine((normal + d) % Dimensions) * base;
      base *= numberOfDoFsPerAxisInPatch;
    }

    for (int unknown = 0; unknown < unknowns; unknown++) {
      fineGridFaceValues[fineIndex * unknowns + unknown] = 0.0;
    }

    int columnStart = rowIndices[rowIndicesStart + row];
    int columnEnd   = rowIndices[rowIndicesStart + row + 1];

    for (int column = columnStart; column < columnEnd; column++) {
      for (int unknown = 0; unknown < unknowns; unknown++) {
        fineGridFaceValues[fineIndex * unknowns + unknown]
          += interpolationData[dataStart + column]
             * rearrangedCoarseData
               [(columnIndices[dataStart + column]) * unknowns + unknown];
      }
    }
  }

  delete[] rearrangedCoarseData;
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__ normalInterpolationMatrix1d,
  const double* __restrict__ tangentialInterpolationMatrix1d,
  const double* __restrict__ coarseGridFaceValues,
  double* __restrict__ fineGridFaceValues
) {
  logTraceInWith3Arguments(
    "interpolateHaloLayer_AoS_tensor_product(...)",
    marker.toString(),
    numberOfDoFsPerAxisInPatch,
    overlap
  );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  dfore(kFine, numberOfDoFsPerAxisInPatch, normal, 0) {
    for (int iFine = 0; iFine < overlap; iFine++) {
      tarch::la::Vector<Dimensions, int> dest = kFine;
      dest(normal
      ) = (marker.getSelectedFaceNumber() < Dimensions)
            ? iFine
            : 2 * overlap - iFine - 1;

      int destLinearised = serialiseVoxelIndexInOverlap(
        dest,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );

      logDebug(
        "interpolateHaloLayer_AoS_tensor_product(...)",
        "clear dof " << dest
      );
      for (int unknown = 0; unknown < unknowns; unknown++) {
        fineGridFaceValues[destLinearised * unknowns + unknown] = 0.0;
      }

      dfore(kCoarse, numberOfDoFsPerAxisInPatch, normal, 0) {
        for (int iCoarse = 0; iCoarse < 2 * overlap; iCoarse++) {
          tarch::la::Vector<Dimensions, int> src = kCoarse;
          src(normal
          )                 = (marker.getSelectedFaceNumber() < Dimensions)
                                ? iCoarse
                                : 2 * overlap - iCoarse - 1;
          int srcLinearised = serialiseVoxelIndexInOverlap(
            src,
            numberOfDoFsPerAxisInPatch,
            overlap,
            normal
          );

          double weight = 1.0;
          for (int d = 0; d < Dimensions; d++) {
            if (d == normal) {
              int col = iCoarse;
              int row = iFine;

              assertion4(col >= 0, row, col, src, dest);
              assertion4(row >= 0, row, col, src, dest);
              int index = col + row * 2 * overlap;
              logDebug(
                "interpolateHaloLayer_AoS_tensor_product(...)",
                "(" << row << "," << col << ")=" << index
                    << " (normal contribution)"
              );
              weight *= normalInterpolationMatrix1d[index];
              assertion(weight == weight);
            } else {
              int col = src(d);
              int row = dest(d)
                        + marker.getRelativePositionWithinFatherCell()(d
                          ) * numberOfDoFsPerAxisInPatch;
              assertion4(col >= 0, row, col, src, dest);
              assertion4(row >= 0, row, col, src, dest);
              int index = col + row * numberOfDoFsPerAxisInPatch;
              logDebug(
                "interpolateHaloLayer_AoS_tensor_product(...)",
                "(" << row << "," << col << ")=" << index
                    << " (tangential contribution)"
              );
              weight *= tangentialInterpolationMatrix1d[index];
              assertion(weight == weight);
            }
          }

          logDebug(
            "interpolateHaloLayer_AoS_tensor_product(...)",
            "add dof " << src << " to " << dest << " with weight " << weight
          );

          for (int unknown = 0; unknown < unknowns; unknown++) {
            fineGridFaceValues[destLinearised * unknowns + unknown]
              += weight
                 * coarseGridFaceValues[srcLinearised * unknowns + unknown];
          }
        }
      }
    }
  }

  logTraceOut("interpolateHaloLayer_AoS_tensor_product(...)");
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int overlap,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ normalInterpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__ tangentialInterpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] const double* __restrict__ coarseGridFaceValues,
  [[maybe_unused]] double* __restrict__ fineGridFaceValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_matrix(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int overlap,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ interpolationData,
  [[maybe_unused]] const int* __restrict__ columnIndices,
  [[maybe_unused]] const int* __restrict__ rowIndices,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] const double* __restrict__ coarseGridFaceValues,
  [[maybe_unused]] double* __restrict__ fineGridFaceValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_second_order(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int overlap,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] const double* __restrict__ coarseGridFaceValues,
  [[maybe_unused]] double* __restrict__ fineGridFaceValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateHaloLayer_AoS_third_order(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int overlap,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] const double* __restrict__ coarseGridFaceValues,
  [[maybe_unused]] double* __restrict__ fineGridFaceValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateCell_AoS_tensor_product(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ interpolationMatrix1d,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] double* __restrict__ fineGridCellValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateCell_AoS_matrix(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ interpolationData,
  [[maybe_unused]] const int* __restrict__ columnIndices,
  [[maybe_unused]] const int* __restrict__ rowIndices,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] double* __restrict__ fineGridCellValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateCell_AoS_second_order(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] double* __restrict__ fineGridCellValues
) {
  assertion(false);
}

void toolbox::blockstructured::interpolateCell_AoS_third_order(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int unknowns,
  [[maybe_unused]] const double* __restrict__ coarseGridCellValues,
  [[maybe_unused]] double* __restrict__ fineGridCellValues
) {
  assertion(false);
}
