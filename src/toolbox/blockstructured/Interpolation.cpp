// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Interpolation.h"

tarch::logging::Log toolbox::blockstructured::_log("toolbox::blockstructured");
tarch::multicore::BooleanSemaphore toolbox::blockstructured::interpolationMapSemaphore;

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

#if DIMENSIONS == 3
  tarch::la::DynamicMatrix* P = new tarch::la::DynamicMatrix(P1d, P1d, false);
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

#if DIMENSIONS == 3
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
#if DIMENSIONS == 3
  return new tarch::la::DynamicMatrix(P2d, P1d, false);
#else
  return new tarch::la::DynamicMatrix(P2d);
#endif
}
