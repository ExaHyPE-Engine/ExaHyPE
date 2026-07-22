// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Riemann.h"

#include "exahype2/fv/VolumeIndex.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "toolbox/blockstructured/Enumeration.h"

void exahype2::dg::interpolateProjectedFacePolynomial(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       order,
  int                                       numberOfProjectedQuantities,
  int                                       unknowns,
  int                                       auxiliaryVariables,
  const double* NOALIAS                     InterpolationMatrix1d,
  const double* NOALIAS                     coarseGridFaceQ,
  double* NOALIAS                           fineGridFaceQ
) {
  const int normal = marker.getSelectedFaceNumber() % DIMENSIONS;
  const int stride = unknowns + auxiliaryVariables;

  const int numberOfDofsPerAxis = order + 1;

  // The half of the hanging face the fine cells cannot supply: it belongs to the coarse
  // neighbour, whose trace lives on the coarse face.
  const int coarseSideFace = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

  peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& fineGridDoF) {
    tarch::la::Vector<DIMENSIONS, int> shiftedFineGridDoF = fineGridDoF;
    for (int leftRight = coarseSideFace * numberOfProjectedQuantities;
         leftRight < (coarseSideFace + 1) * numberOfProjectedQuantities;
         leftRight++) {
      shiftedFineGridDoF(normal) = leftRight;
      const int fineOffset       = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
                               shiftedFineGridDoF,
                               order + 1,
                               numberOfProjectedQuantities,
                               normal
                             )
        * stride;
      std::fill_n(fineGridFaceQ + fineOffset, stride, 0.0);
    }

    peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& coarseGridDoF) {
      double tensorProductWeight = 1.0;
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          const int subMatrix = marker.getRelativePositionWithinFatherCell()(d);
          assertion(subMatrix >= 0);
          assertion(subMatrix <= 2);

          // InterpolationMatrix1d[b][fine][coarse] evaluates the coarse shape function at
          // the fine node of child interval b.
          const int linearisedMatrixIndex = subMatrix * numberOfDofsPerAxis * numberOfDofsPerAxis
            + fineGridDoF(d) * numberOfDofsPerAxis + coarseGridDoF(d);
          assertion(linearisedMatrixIndex >= 0);
          assertion(
            linearisedMatrixIndex < 3 * numberOfDofsPerAxis * numberOfDofsPerAxis,
            linearisedMatrixIndex,
            numberOfDofsPerAxis
          );
          tensorProductWeight *= InterpolationMatrix1d[linearisedMatrixIndex];
        }
      }

      tarch::la::Vector<DIMENSIONS, int> shiftedCoarseGridDoF = coarseGridDoF;
      for (int leftRight = coarseSideFace * numberOfProjectedQuantities;
           leftRight < (coarseSideFace + 1) * numberOfProjectedQuantities;
           leftRight++) {
        shiftedFineGridDoF(normal)   = leftRight;
        shiftedCoarseGridDoF(normal) = leftRight;
        const int fineOffset         = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
                                 shiftedFineGridDoF,
                                 order + 1,
                                 numberOfProjectedQuantities,
                                 normal
                               )
          * stride;
        const int coarseOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
                                   shiftedCoarseGridDoF,
                                   order + 1,
                                   numberOfProjectedQuantities,
                                   normal
                                 )
          * stride;

        for (int unknown = 0; unknown < stride; unknown++) {
          fineGridFaceQ[fineOffset + unknown] += tensorProductWeight * coarseGridFaceQ[coarseOffset + unknown];
        }
      }
    });
  });
}

void exahype2::dg::restrictAndAccumulateProjectedFacePolynomial(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       order,
  int                                       numberOfProjectedQuantities,
  int                                       unknowns,
  int                                       auxiliaryVariables,
  const double* NOALIAS                     RestrictionMatrix1d,
  const double* NOALIAS                     QuadratureWeights1d,
  const double* NOALIAS                     fineGridFaceQ,
  double* NOALIAS                           coarseGridFaceQ
) {
  assertion(numberOfProjectedQuantities >= 1);
  // @todo Remove this restriction. The routine should work for other values as
  //  well, but that is untested.
  assertion((numberOfProjectedQuantities) == (1), numberOfProjectedQuantities, 1);
  const int normal = marker.getSelectedFaceNumber() % DIMENSIONS;
  const int stride = unknowns + auxiliaryVariables;

  const int numberOfDofsPerAxis = order + 1;

  const int leftRightFace = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
  peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& fineGridDoF) {
    peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& coarseGridDoF) {
      double tensorProductWeight = 1.0;
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          const int subMatrix = marker.getRelativePositionWithinFatherCell()(d);
          assertion(subMatrix >= 0);
          assertion(subMatrix <= 2);

          // RestrictionMatrix1d[b][coarse][fine] evaluates the coarse shape function at the
          // fine node of child interval b over the child width. The quadrature weight ratio
          // turns the sum over the children into the mass-lumped L2 projection of the fine
          // trace, so every polynomial the coarse face basis represents is reproduced.
          const int linearisedMatrixIndex = subMatrix * numberOfDofsPerAxis * numberOfDofsPerAxis
            + coarseGridDoF(d) * numberOfDofsPerAxis + fineGridDoF(d);
          assertion(linearisedMatrixIndex >= 0);
          assertion(
            linearisedMatrixIndex < 3 * numberOfDofsPerAxis * numberOfDofsPerAxis,
            linearisedMatrixIndex,
            numberOfDofsPerAxis
          );
          tensorProductWeight *= RestrictionMatrix1d[linearisedMatrixIndex] * QuadratureWeights1d[fineGridDoF(d)]
            / QuadratureWeights1d[coarseGridDoF(d)];
        }
      }

      tarch::la::Vector<DIMENSIONS, int> shiftedFineGridDoF   = fineGridDoF;
      tarch::la::Vector<DIMENSIONS, int> shiftedCoarseGridDoF = coarseGridDoF;
      for (int leftRight = leftRightFace * numberOfProjectedQuantities;
           leftRight < (leftRightFace + 1) * numberOfProjectedQuantities;
           leftRight++)
        for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
          shiftedFineGridDoF(normal)   = leftRight;
          shiftedCoarseGridDoF(normal) = leftRight;
          const int coarseOffset =
            toolbox::blockstructured::serialiseVoxelIndexInOverlap(
              shiftedCoarseGridDoF,
              order + 1,
              numberOfProjectedQuantities,
              normal
            ) * stride
            + unknown;
          const int fineOffset =
            toolbox::blockstructured::serialiseVoxelIndexInOverlap(
              shiftedFineGridDoF,
              order + 1,
              numberOfProjectedQuantities,
              normal
            ) * stride
            + unknown;

          assertion(coarseOffset >= 0);
          assertion(fineOffset >= 0);

          coarseGridFaceQ[coarseOffset] += tensorProductWeight * fineGridFaceQ[fineOffset];
        }
    });
  });
}

void exahype2::dg::interpolateRiemannSolution(
  int                                       faceNumber,
  const tarch::la::Vector<DIMENSIONS, int>& positionWithinFatherCell,
  int                                       order,
  int                                       unknowns,
  int                                       auxiliaryVariables,
  double                                    maxEigenvalue,
  const double* NOALIAS                     InterpolationMatrix1d,
  const double* NOALIAS                     coarseGridFaceRiemannSolution,
  const double* NOALIAS                     coarseGridFaceProjection,
  const double* NOALIAS                     fineGridFaceProjection,
  double* NOALIAS                           fineGridFaceRiemannSolution
) {
  const int normal              = faceNumber % DIMENSIONS;
  const int numberOfDofsPerAxis = order + 1;
  const int projectionStride    = unknowns + auxiliaryVariables;

  // The coarse neighbour's face integral consumes the layer on its side of the
  // coarse face; the fine cell's face integral consumes the opposite layer of
  // the hanging face. Evaluating the one polynomial into the other layer makes
  // both sides integrate the same flux.
  const int coarseSideLayer = faceNumber < DIMENSIONS ? 0 : 1;
  const int fineSideLayer   = 1 - coarseSideLayer;

  // Sign of the jump penalty: an increase of the fine trace has to reduce the
  // net inflow of the fine cell, and the face integral consumes the flux with
  // opposite signs on the two sides of the cell.
  const double penaltyOrientation = faceNumber < DIMENSIONS ? 1.0 : -1.0;

  peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& fineGridDoF) {
    tarch::la::Vector<DIMENSIONS, int> shiftedFineGridDoF = fineGridDoF;
    shiftedFineGridDoF(normal)                            = fineSideLayer;
    const int fineOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedFineGridDoF, numberOfDofsPerAxis, 1, normal)
      * unknowns;
    const int fineProjectionOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedFineGridDoF, numberOfDofsPerAxis, 1, normal)
      * projectionStride;

    // The penalty damps the fine trace against the restriction's image on the
    // coarse face: that difference is the part of the fine trace the coarse
    // flux cannot see and would otherwise never dissipate, and it integrates
    // to zero over the coarse face, so the transition stays conservative.
    for (int unknown = 0; unknown < unknowns; unknown++) {
      fineGridFaceRiemannSolution[fineOffset + unknown] =
        -penaltyOrientation * 0.5 * maxEigenvalue * fineGridFaceProjection[fineProjectionOffset + unknown];
    }

    peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& coarseGridDoF) {
      double tensorProductWeight = 1.0;
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != normal) {
          const int subMatrix = positionWithinFatherCell(d);
          assertion(subMatrix >= 0);
          assertion(subMatrix <= 2);

          // InterpolationMatrix1d[b][fine][coarse] evaluates the coarse shape function at
          // the fine node of child interval b.
          const int linearisedMatrixIndex = subMatrix * numberOfDofsPerAxis * numberOfDofsPerAxis
            + fineGridDoF(d) * numberOfDofsPerAxis + coarseGridDoF(d);
          assertion(linearisedMatrixIndex >= 0);
          assertion(
            linearisedMatrixIndex < 3 * numberOfDofsPerAxis * numberOfDofsPerAxis,
            linearisedMatrixIndex,
            numberOfDofsPerAxis
          );
          tensorProductWeight *= InterpolationMatrix1d[linearisedMatrixIndex];
        }
      }

      tarch::la::Vector<DIMENSIONS, int> shiftedCoarseGridDoF = coarseGridDoF;
      shiftedCoarseGridDoF(normal)                            = coarseSideLayer;
      const int coarseOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedCoarseGridDoF, numberOfDofsPerAxis, 1, normal)
        * unknowns;

      // The restricted fine trace lives in the coarse face's other layer.
      tarch::la::Vector<DIMENSIONS, int> shiftedRestrictedDoF = coarseGridDoF;
      shiftedRestrictedDoF(normal)                            = fineSideLayer;
      const int restrictedOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(shiftedRestrictedDoF, numberOfDofsPerAxis, 1, normal)
        * projectionStride;

      for (int unknown = 0; unknown < unknowns; unknown++) {
        fineGridFaceRiemannSolution[fineOffset + unknown] += tensorProductWeight
          * (coarseGridFaceRiemannSolution[coarseOffset + unknown]
             + penaltyOrientation * 0.5 * maxEigenvalue * coarseGridFaceProjection[restrictedOffset + unknown]);
      }
    });
  });
}

void exahype2::dg::clearSolutionProjection(
  int             order,
  int             unknowns,
  int             auxiliaryVariables,
  int             numberOfProjectedQuantities,
  double* NOALIAS faceQ
) {
#if DIMENSIONS == 2
  const int numberOfDoFs = (order + 1);
#else
  const int numberOfDoFs = (order + 1) * (order + 1);
#endif
  const int numberOfDoubles = numberOfDoFs * (unknowns + auxiliaryVariables) * 2 * numberOfProjectedQuantities;
  std::fill_n(faceQ, numberOfDoubles, 0.0);
}

void exahype2::dg::projectVolumetricDataOntoFaces(
  [[maybe_unused]] const double* NOALIAS       cellQ,
  [[maybe_unused]] int                         order,
  [[maybe_unused]] int                         unknowns,
  [[maybe_unused]] int                         auxiliaryVariables,
  [[maybe_unused]] const double* const NOALIAS BasisFunctionValuesLeft,
  [[maybe_unused]] double* const NOALIAS       faceQLeft,
  [[maybe_unused]] double* const NOALIAS       faceQRight,
  [[maybe_unused]] double* const NOALIAS       faceQBottom,
  [[maybe_unused]] double* const NOALIAS       faceQUp
) {
#if DIMENSIONS == 2
  const int stride = unknowns + auxiliaryVariables;

  // Project left/right
  for (int y = 0; y < order + 1; y++) {
    const auto left           = fv::toVolumeIndex(1, y);
    const auto right          = fv::toVolumeIndex(0, y);
    const int  leftFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(left, order + 1, 1, 0) * stride;
    const int rightFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(right, order + 1, 1, 0) * stride;
    for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
      faceQLeft[leftFaceOffset + unknown]   = 0.0;
      faceQRight[rightFaceOffset + unknown] = 0.0;
    }
  }

  for (int y = 0; y < order + 1; y++) {
    const auto left           = fv::toVolumeIndex(1, y);
    const auto right          = fv::toVolumeIndex(0, y);
    const int  leftFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(left, order + 1, 1, 0) * stride;
    const int rightFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(right, order + 1, 1, 0) * stride;
    for (int x = 0; x < order + 1; x++) {
      const int cellOffset = peano4::dLinearised(fv::toVolumeIndex(x, y), order + 1) * stride;
      for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
        faceQLeft[leftFaceOffset + unknown] += BasisFunctionValuesLeft[x] * cellQ[cellOffset + unknown];
        faceQRight[rightFaceOffset + unknown] += BasisFunctionValuesLeft[order - x] * cellQ[cellOffset + unknown];
      }
    }
  }

  // Project bottom/up
  for (int x = 0; x < order + 1; x++) {
    const auto bottom = fv::toVolumeIndex(x, 1);
    const auto up     = fv::toVolumeIndex(x, 0);
    const int  bottomFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottom, order + 1, 1, 1) * stride;
    const int upFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(up, order + 1, 1, 1) * stride;
    for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
      faceQBottom[bottomFaceOffset + unknown] = 0.0;
      faceQUp[upFaceOffset + unknown]         = 0.0;
    }
  }

  for (int x = 0; x < order + 1; x++) {
    const auto bottom = fv::toVolumeIndex(x, 1);
    const auto up     = fv::toVolumeIndex(x, 0);
    const int  bottomFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottom, order + 1, 1, 1) * stride;
    const int upFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(up, order + 1, 1, 1) * stride;
    for (int y = 0; y < order + 1; y++) {
      const int cellOffset = peano4::dLinearised(fv::toVolumeIndex(x, y), order + 1) * stride;
      for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
        faceQBottom[bottomFaceOffset + unknown] += BasisFunctionValuesLeft[y] * cellQ[cellOffset + unknown];
        faceQUp[upFaceOffset + unknown] += BasisFunctionValuesLeft[order - y] * cellQ[cellOffset + unknown];
      }
    }
  }
#endif
}

void exahype2::dg::projectVolumetricDataOntoFaces(
  [[maybe_unused]] const double* NOALIAS       cellQ,
  [[maybe_unused]] int                         order,
  [[maybe_unused]] int                         unknowns,
  [[maybe_unused]] int                         auxiliaryVariables,
  [[maybe_unused]] const double* const NOALIAS BasisFunctionValuesLeft,
  [[maybe_unused]] double* const NOALIAS       faceQLeft,
  [[maybe_unused]] double* const NOALIAS       faceQRight,
  [[maybe_unused]] double* const NOALIAS       faceQBottom,
  [[maybe_unused]] double* const NOALIAS       faceQUp,
  [[maybe_unused]] double* const NOALIAS       faceQFront,
  [[maybe_unused]] double* const NOALIAS       faceQBack
) {
#if DIMENSIONS == 3
  const int stride = unknowns + auxiliaryVariables;

  // Project left/right
  for (int y = 0; y < order + 1; y++) {
    for (int z = 0; z < order + 1; z++) {
      const auto left          = fv::toVolumeIndex(1, y, z);
      const auto right         = fv::toVolumeIndex(0, y, z);
      const int leftFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(left, order + 1, 1, 0) * stride;
      const int rightFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(right, order + 1, 1, 0) * stride;
      for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
        faceQLeft[leftFaceOffset + unknown]   = 0.0;
        faceQRight[rightFaceOffset + unknown] = 0.0;
      }
    }
  }

  for (int y = 0; y < order + 1; y++) {
    for (int z = 0; z < order + 1; z++) {
      const auto left          = fv::toVolumeIndex(1, y, z);
      const auto right         = fv::toVolumeIndex(0, y, z);
      const int leftFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(left, order + 1, 1, 0) * stride;
      const int rightFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(right, order + 1, 1, 0) * stride;
      for (int x = 0; x < order + 1; x++) {
        const int cellOffset = peano4::dLinearised(fv::toVolumeIndex(x, y, z), order + 1) * stride;
        for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
          faceQLeft[leftFaceOffset + unknown] += BasisFunctionValuesLeft[x] * cellQ[cellOffset + unknown];
          faceQRight[rightFaceOffset + unknown] += BasisFunctionValuesLeft[order - x] * cellQ[cellOffset + unknown];
        }
      }
    }
  }

  // Project bottom/up
  for (int x = 0; x < order + 1; x++) {
    for (int z = 0; z < order + 1; z++) {
      const auto bottom = fv::toVolumeIndex(x, 1, z);
      const auto up     = fv::toVolumeIndex(x, 0, z);
      const int  bottomFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottom, order + 1, 1, 1) * stride;
      const int upFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(up, order + 1, 1, 1) * stride;
      for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
        faceQBottom[bottomFaceOffset + unknown] = 0.0;
        faceQUp[upFaceOffset + unknown]         = 0.0;
      }
    }
  }

  for (int x = 0; x < order + 1; x++) {
    for (int z = 0; z < order + 1; z++) {
      const auto bottom = fv::toVolumeIndex(x, 1, z);
      const auto up     = fv::toVolumeIndex(x, 0, z);
      const int  bottomFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottom, order + 1, 1, 1) * stride;
      const int upFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(up, order + 1, 1, 1) * stride;
      for (int y = 0; y < order + 1; y++) {
        const int cellOffset = peano4::dLinearised(fv::toVolumeIndex(x, y, z), order + 1) * stride;
        for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
          faceQBottom[bottomFaceOffset + unknown] += BasisFunctionValuesLeft[y] * cellQ[cellOffset + unknown];
          faceQUp[upFaceOffset + unknown] += BasisFunctionValuesLeft[order - y] * cellQ[cellOffset + unknown];
        }
      }
    }
  }

  // Project front/back
  for (int x = 0; x < order + 1; x++) {
    for (int y = 0; y < order + 1; y++) {
      const auto front = fv::toVolumeIndex(x, y, 1);
      const auto back  = fv::toVolumeIndex(x, y, 0);
      const int  frontFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(front, order + 1, 1, 2) * stride;
      const int backFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(back, order + 1, 1, 2) * stride;
      for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
        faceQFront[frontFaceOffset + unknown] = 0.0;
        faceQBack[backFaceOffset + unknown]   = 0.0;
      }
    }
  }

  for (int x = 0; x < order + 1; x++) {
    for (int y = 0; y < order + 1; y++) {
      const auto front = fv::toVolumeIndex(x, y, 1);
      const auto back  = fv::toVolumeIndex(x, y, 0);
      const int  frontFaceOffset =
        toolbox::blockstructured::serialiseVoxelIndexInOverlap(front, order + 1, 1, 2) * stride;
      const int backFaceOffset = toolbox::blockstructured::serialiseVoxelIndexInOverlap(back, order + 1, 1, 2) * stride;
      for (int z = 0; z < order + 1; z++) {
        const int cellOffset = peano4::dLinearised(fv::toVolumeIndex(x, y, z), order + 1) * stride;
        for (int unknown = 0; unknown < unknowns + auxiliaryVariables; unknown++) {
          faceQFront[frontFaceOffset + unknown] += BasisFunctionValuesLeft[z] * cellQ[cellOffset + unknown];
          faceQBack[backFaceOffset + unknown] += BasisFunctionValuesLeft[order - z] * cellQ[cellOffset + unknown];
        }
      }
    }
  }
#endif
}

void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  [[maybe_unused]] const double* NOALIAS       cellQ,
  [[maybe_unused]] int                         order,
  [[maybe_unused]] int                         unknowns,
  [[maybe_unused]] int                         auxiliaryVariables,
  [[maybe_unused]] const double* const NOALIAS BasisFunctionValuesLeft,
  [[maybe_unused]] double* const NOALIAS       faceQLeft,
  [[maybe_unused]] double* const NOALIAS       faceQRight,
  [[maybe_unused]] double* const NOALIAS       faceQBottom,
  [[maybe_unused]] double* const NOALIAS       faceQUp
) {
  assertion(false, "Not implemented yet");
}

void exahype2::dg::projectVolumetricDataAndGradientOntoFaces(
  [[maybe_unused]] const double* NOALIAS       cellQ,
  [[maybe_unused]] int                         order,
  [[maybe_unused]] int                         unknowns,
  [[maybe_unused]] int                         auxiliaryVariables,
  [[maybe_unused]] const double* const NOALIAS BasisFunctionValuesLeft,
  [[maybe_unused]] double* const NOALIAS       faceQLeft,
  [[maybe_unused]] double* const NOALIAS       faceQRight,
  [[maybe_unused]] double* const NOALIAS       faceQBottom,
  [[maybe_unused]] double* const NOALIAS       faceQUp,
  [[maybe_unused]] double* const NOALIAS       faceQFront,
  [[maybe_unused]] double* const NOALIAS       faceQBack
) {
  assertion(false, "Not implemented yet");
}

void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  [[maybe_unused]] const double* const NOALIAS         faceQLeft,
  [[maybe_unused]] const double* const NOALIAS         faceQRight,
  [[maybe_unused]] const double* const NOALIAS         faceQBottom,
  [[maybe_unused]] const double* const NOALIAS         faceQUp,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] const int                           auxiliaryVariables,
  [[maybe_unused]] const tarch::la::Vector<2, double>& cellSize,
  [[maybe_unused]] const double* const NOALIAS         BasisFunctionValuesLeft,
  [[maybe_unused]] const double* NOALIAS               MassMatrixDiagonal1d,
  [[maybe_unused]] double* NOALIAS                     cellQ
) {
#if DIMENSIONS == 2
  const int stride = unknowns;

  peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    const int    cellOffset = peano4::dLinearised(dof, order + 1) * stride;
    const double coeffLeft  = MassMatrixDiagonal1d[dof[1]] * BasisFunctionValuesLeft[dof[0]] * cellSize(1);
    const double coeffRight = MassMatrixDiagonal1d[dof[1]] * BasisFunctionValuesLeft[order - dof[0]] * cellSize(1);

    const double coeffBottom = MassMatrixDiagonal1d[dof[0]] * BasisFunctionValuesLeft[dof[1]] * cellSize(0);
    const double coeffUp     = MassMatrixDiagonal1d[dof[0]] * BasisFunctionValuesLeft[order - dof[1]] * cellSize(0);

    tarch::la::Vector<DIMENSIONS, int> leftDoF   = dof;
    tarch::la::Vector<DIMENSIONS, int> rightDoF  = dof;
    tarch::la::Vector<DIMENSIONS, int> bottomDoF = dof;
    tarch::la::Vector<DIMENSIONS, int> upperDoF  = dof;

    leftDoF(0)   = 1;
    rightDoF(0)  = 0;
    bottomDoF(1) = 1;
    upperDoF(1)  = 0;
    const int leftFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(leftDoF, order + 1, 1, 0) * stride;
    const int rightFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(rightDoF, order + 1, 1, 0) * stride;
    const int bottomFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottomDoF, order + 1, 1, 1) * stride;
    const int upperFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(upperDoF, order + 1, 1, 1) * stride;

    for (int var = 0; var < unknowns; var++) {
      cellQ[cellOffset + var] += coeffLeft * faceQLeft[leftFaceOffset + var]
        - coeffRight * faceQRight[rightFaceOffset + var] + coeffBottom * faceQBottom[bottomFaceOffset + var]
        - coeffUp * faceQUp[upperFaceOffset + var];
    }
  });
#endif
}

void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  [[maybe_unused]] const double* const NOALIAS         faceQLeft,
  [[maybe_unused]] const double* const NOALIAS         faceQRight,
  [[maybe_unused]] const double* const NOALIAS         faceQBottom,
  [[maybe_unused]] const double* const NOALIAS         faceQUp,
  [[maybe_unused]] const double* const NOALIAS         faceQFront,
  [[maybe_unused]] const double* const NOALIAS         faceQBack,
  [[maybe_unused]] int                                 order,
  [[maybe_unused]] int                                 unknowns,
  [[maybe_unused]] const int                           auxiliaryVariables,
  [[maybe_unused]] const tarch::la::Vector<3, double>& cellSize,
  [[maybe_unused]] const double* const NOALIAS         BasisFunctionValuesLeft,
  [[maybe_unused]] const double* NOALIAS               MassMatrixDiagonal1d,
  [[maybe_unused]] double* NOALIAS                     cellQ
) {
#if DIMENSIONS == 3
  const int stride = unknowns;

  peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    const int cellOffset = peano4::dLinearised(dof, order + 1) * stride;
    // coefficients of faces in x direction
    const double coeffLeft = BasisFunctionValuesLeft[dof[0]] * MassMatrixDiagonal1d[dof[1]]
      * MassMatrixDiagonal1d[dof[2]] * cellSize(1) * cellSize(2);
    const double coeffRight = BasisFunctionValuesLeft[order - dof[0]] * MassMatrixDiagonal1d[dof[1]]
      * MassMatrixDiagonal1d[dof[2]] * cellSize(1) * cellSize(2);
    // coefficients of faces in y direction
    const double coeffBottom = MassMatrixDiagonal1d[dof[0]] * BasisFunctionValuesLeft[dof[1]]
      * MassMatrixDiagonal1d[dof[2]] * cellSize(0) * cellSize(2);
    const double coeffUp = MassMatrixDiagonal1d[dof[0]] * BasisFunctionValuesLeft[order - dof[1]]
      * MassMatrixDiagonal1d[dof[2]] * cellSize(0) * cellSize(2);
    // coefficients of faces in z direction
    const double coeffFront = MassMatrixDiagonal1d[dof[0]] * MassMatrixDiagonal1d[dof[1]]
      * BasisFunctionValuesLeft[dof[2]] * cellSize(0) * cellSize(1);
    const double coeffBack = MassMatrixDiagonal1d[dof[0]] * MassMatrixDiagonal1d[dof[1]]
      * BasisFunctionValuesLeft[order - dof[2]] * cellSize(0) * cellSize(1);

    tarch::la::Vector<DIMENSIONS, int> leftDoF   = dof;
    tarch::la::Vector<DIMENSIONS, int> rightDoF  = dof;
    tarch::la::Vector<DIMENSIONS, int> frontDoF  = dof;
    tarch::la::Vector<DIMENSIONS, int> backDoF   = dof;
    tarch::la::Vector<DIMENSIONS, int> bottomDoF = dof;
    tarch::la::Vector<DIMENSIONS, int> upperDoF  = dof;

    leftDoF(0)   = 1;
    rightDoF(0)  = 0;
    bottomDoF(1) = 1;
    upperDoF(1)  = 0;
    frontDoF(2)  = 1;
    backDoF(2)   = 0;
    const int leftFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(leftDoF, order + 1, 1, 0) * stride;
    const int rightFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(rightDoF, order + 1, 1, 0) * stride;
    const int bottomFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(bottomDoF, order + 1, 1, 1) * stride;
    const int upperFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(upperDoF, order + 1, 1, 1) * stride;
    const int frontFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(frontDoF, order + 1, 1, 2) * stride;
    const int backFaceOffset =
      toolbox::blockstructured::serialiseVoxelIndexInOverlap(backDoF, order + 1, 1, 2) * stride;

    for (int var = 0; var < unknowns; var++) {
      cellQ[cellOffset + var] += +coeffLeft * faceQLeft[leftFaceOffset + var]
        - coeffRight * faceQRight[rightFaceOffset + var] + coeffBottom * faceQBottom[bottomFaceOffset + var]
        - coeffUp * faceQUp[upperFaceOffset + var] + coeffFront * faceQFront[frontFaceOffset + var]
        - coeffBack * faceQBack[backFaceOffset + var];
    }
  });
#endif
}
