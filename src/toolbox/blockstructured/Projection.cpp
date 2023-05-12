#include "Projection.h"

#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "exahype2/fd/BoundaryConditions.h"
#include "exahype2/enumerator/Enumerator.h"


void toolbox::blockstructured::projectPatchSolutionOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* __restrict__ leftFace,
    double* __restrict__ bottomFace,
    double* __restrict__ rightFace,
    double* __restrict__ topFace,
    bool permuteInnerAndOuterSideOfFace
) {
  assertionEquals( Dimensions, 2 );
  double* faces[4] = { leftFace, bottomFace, rightFace, topFace };
  projectPatchSolutionOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces,
    permuteInnerAndOuterSideOfFace
  );
}


void toolbox::blockstructured::projectPatchSolutionOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* __restrict__ leftFace,
    double* __restrict__ bottomFace,
    double* __restrict__ frontFace,
    double* __restrict__ rightFace,
    double* __restrict__ topFace,
    double* __restrict__ backFace,
    bool permuteInnerAndOuterSideOfFace
) {
  assertionEquals( Dimensions, 3 );
  double* faces[6] = { leftFace, bottomFace, frontFace, rightFace, topFace, backFace};
  projectPatchSolutionOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces,
    permuteInnerAndOuterSideOfFace
  );
}


void toolbox::blockstructured::projectPatchSolutionOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* faces[2*Dimensions],
    bool permuteInnerAndOuterSideOfFace
) {
  for(int d=0; d<Dimensions; d++) {
    /**
     * d-loop over all dimensions except d. The vector k's entry d is set
     * to 0. We start with the left/bottom face, i.e. the one closer to the
     * coordinate system's origin.
     */
    dfore(k,numberOfVolumesPerAxisInPatch,d,0) {
      for (int i=0; i<haloSize; i++) {
        tarch::la::Vector<Dimensions,int> patchCell   = k;
        tarch::la::Vector<Dimensions,int> overlapCell = k;
        patchCell(d)   = i;
        if (permuteInnerAndOuterSideOfFace) {
          overlapCell(d) = i;
        }
        else {
          overlapCell(d) = i+haloSize;
        }

        int patchCellSerialised   = peano4::utils::dLinearised(patchCell,12);
        int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,12,1,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }

        patchCell(d)   = i+numberOfVolumesPerAxisInPatch-haloSize;
        if (permuteInnerAndOuterSideOfFace) {
          overlapCell(d) = i+haloSize;
        }
        else {
          overlapCell(d) = i;
        }

        patchCellSerialised   = peano4::utils::dLinearised(patchCell,12);
        overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,12,1,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }
      }
    }
  }
}
