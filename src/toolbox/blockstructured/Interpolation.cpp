// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Interpolation.h"

tarch::multicore::BooleanSemaphore toolbox::blockstructured::interpolationMapSemaphore;

namespace {
  void clearInterpolationMap(toolbox::blockstructured::internal::FaceInterpolationMap& map) {
    for (auto& entry : map) {
      delete entry.second;
    }
    map.clear();
  }

  void clearInterpolationMap(toolbox::blockstructured::internal::CellInterpolationMap& map) {
    for (auto& entry : map) {
      delete entry.second;
    }
    map.clear();
  }
} // namespace

toolbox::blockstructured::internal::FaceInterpolationMap& toolbox::blockstructured::internal::
  getPiecewiseConstantFaceInterpolationMap() {
  static FaceInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::FaceInterpolationMap& toolbox::blockstructured::internal::
  getLinearWithConstantExtrapolationFaceInterpolationMap() {
  static FaceInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::FaceInterpolationMap& toolbox::blockstructured::internal::
  getLinearWithLinearExtrapolationFaceInterpolationMap() {
  static FaceInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::FaceInterpolationMap& toolbox::blockstructured::internal::
  getLinearWithConstantExtrapolationAndLinearNormalInterpolationFaceInterpolationMap() {
  static FaceInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::FaceInterpolationMap& toolbox::blockstructured::internal::
  getLinearWithLinearExtrapolationAndLinearNormalInterpolationFaceInterpolationMap() {
  static FaceInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::CellInterpolationMap& toolbox::blockstructured::internal::
  getPiecewiseConstantCellInterpolationMap() {
  static CellInterpolationMap map;
  return map;
}

toolbox::blockstructured::internal::CellInterpolationMap& toolbox::blockstructured::internal::
  getLinearCellInterpolationMap() {
  static CellInterpolationMap map;
  return map;
}

void toolbox::blockstructured::clearInterpolationOperatorCaches() {
  tarch::multicore::Lock lock(interpolationMapSemaphore);

  clearInterpolationMap(internal::getPiecewiseConstantFaceInterpolationMap());
  clearInterpolationMap(internal::getLinearWithConstantExtrapolationFaceInterpolationMap());
  clearInterpolationMap(internal::getLinearWithLinearExtrapolationFaceInterpolationMap());
  clearInterpolationMap(internal::getLinearWithConstantExtrapolationAndLinearNormalInterpolationFaceInterpolationMap());
  clearInterpolationMap(internal::getLinearWithLinearExtrapolationAndLinearNormalInterpolationFaceInterpolationMap());
  clearInterpolationMap(internal::getPiecewiseConstantCellInterpolationMap());
  clearInterpolationMap(internal::getLinearCellInterpolationMap());
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch) {
#if DIMENSIONS == 2
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
    tarch::la::Vector<DIMENSIONS, int>
      sourceCell = (volumeIndex + patchIndex * numberOfDoFsPerAxisInPatch) / 3;
    int sourceCellLinearised = peano4::dLinearised(
      sourceCell,
      numberOfDoFsPerAxisInPatch
    );
    (*result)(currentRow, sourceCellLinearised) = 1.0;
    currentRow++;
  }
  enddforx

    logDebug("created new volumetric matrix for "
        << numberOfDoFsPerAxisInPatch << ": " << result->toString());
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
  assertion((P1d.cols()) == (numberOfDoFsPerAxisInPatch), P1d.cols(), numberOfDoFsPerAxisInPatch, P1d.toString());
  assertion((P1d.rows()) == (3 * numberOfDoFsPerAxisInPatch), P1d.rows(), 3 * numberOfDoFsPerAxisInPatch, P1d.toString());

#if DIMENSIONS == 3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d, P1d);
#else
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d);
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
  logDebug("matrix for normal=" << normal << ": " << P->toString());
  return P;
}

tarch::la::DynamicMatrix* toolbox::blockstructured::internal::
  createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
    const tarch::la::DynamicMatrix& P1d,
    int                             numberOfDoFsPerAxisInPatch,
    int                             normal
  ) {
  assertion((P1d.cols()) == (numberOfDoFsPerAxisInPatch), P1d.cols(), numberOfDoFsPerAxisInPatch, P1d.toString());
  assertion((P1d.rows()) == (3 * numberOfDoFsPerAxisInPatch), P1d.rows(), 3 * numberOfDoFsPerAxisInPatch, P1d.toString());

#if DIMENSIONS == 3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d, P1d);
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
  logDebug("matrix for normal=" << normal << ": " << P->toString());
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

  logDebug("1d matrix: " << P1d.toString());

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

  logDebug("1d interpolation matrix: " << P1d.toString(true));

  tarch::la::DynamicMatrix P2d(P1d, P1d);
  logDebug("2d interpolation matrix: " << P2d.toString(true));

#if DIMENSIONS == 3
  return new tarch::la::DynamicMatrix(P2d, P1d);
#else
  return new tarch::la::DynamicMatrix(P2d);
#endif
}
