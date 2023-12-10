#include "Projection.h"

#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"

#include <iostream>

void toolbox::blockstructured::projectPatchSolutionOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* __restrict__ leftFace,
    double* __restrict__ bottomFace,
    double* __restrict__ rightFace,
    double* __restrict__ topFace
) {
  assertionEquals( Dimensions, 2 );
  double* faces[4] = { leftFace, bottomFace, rightFace, topFace };
  projectPatchSolutionOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
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
    double* __restrict__ backFace
) {
  assertionEquals( Dimensions, 3 );
  double* faces[6] = { leftFace, bottomFace, frontFace, rightFace, topFace, backFace};
  projectPatchSolutionOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
  );
}


void toolbox::blockstructured::projectPatchSolutionOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* faces[2*Dimensions]
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
        overlapCell(d) = i+haloSize;

        int patchCellSerialised   = peano4::utils::dLinearised(patchCell,numberOfVolumesPerAxisInPatch);
        int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
          if (Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j]!=17.0) std::cerr << "not good";
        }

        patchCell(d)   = i+numberOfVolumesPerAxisInPatch-haloSize;
        overlapCell(d) = i;

        patchCellSerialised   = peano4::utils::dLinearised(patchCell,12);
        overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d+Dimensions][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }
      }
    }
  }
}


void toolbox::blockstructured::projectPatchHaloOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* __restrict__ leftFace,
    double* __restrict__ bottomFace,
    double* __restrict__ rightFace,
    double* __restrict__ topFace
) {
  assertionEquals( Dimensions, 2 );
  double* faces[4] = { leftFace, bottomFace, rightFace, topFace };
  projectPatchHaloOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
  );
}


void toolbox::blockstructured::projectPatchHaloOntoFaces(
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
    double* __restrict__ backFace
) {
  assertionEquals( Dimensions, 3 );
  double* faces[6] = { leftFace, bottomFace, frontFace, rightFace, topFace, backFace};
  projectPatchHaloOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
  );
}


void toolbox::blockstructured::projectPatchHaloOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* faces[2*Dimensions]
) {
  for(int d=0; d<Dimensions; d++) {
    /**
     * d-loop over all dimensions except d. The vector k's entry d is set
     * to 0. We start with the left/bottom face, i.e. the one closer to the
     * coordinate system's origin.
     */
    dfore(k,numberOfVolumesPerAxisInPatch,d,0) {
      for (int i=0; i<haloSize; i++) {
        tarch::la::Vector<Dimensions,int> patchCell   = k + tarch::la::Vector<Dimensions,int>(haloSize);
        tarch::la::Vector<Dimensions,int> overlapCell = k;
        patchCell(d)   = i;
        overlapCell(d) = i; // this is correct

        int patchCellSerialised   = peano4::utils::dLinearised(patchCell,numberOfVolumesPerAxisInPatch+2*haloSize);
        int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);

        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }

        patchCell(d)   = i+numberOfVolumesPerAxisInPatch+haloSize;
        overlapCell(d) = i+haloSize;

        patchCellSerialised   = peano4::utils::dLinearised(patchCell,numberOfVolumesPerAxisInPatch+2*haloSize);
        overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d+Dimensions][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }
      }
    }
  }
}


void toolbox::blockstructured::extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* __restrict__ leftFace,
    double* __restrict__ bottomFace,
    double* __restrict__ rightFace,
    double* __restrict__ topFace
) {
  assertionEquals( Dimensions, 2 );
  double* faces[4] = { leftFace, bottomFace, rightFace, topFace };
  extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
  );
}


void toolbox::blockstructured::extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
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
    double* __restrict__ backFace
) {
  assertionEquals( Dimensions, 3 );
  double* faces[6] = { leftFace, bottomFace, frontFace, rightFace, topFace, backFace};
  extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
    numberOfVolumesPerAxisInPatch,
    haloSize,
    unknowns,
    auxiliaryVariables,
    Q,
    faces
  );
}


void toolbox::blockstructured::extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
    int    numberOfVolumesPerAxisInPatch,
    int    haloSize,
    int    unknowns,
    int    auxiliaryVariables,
    const double* __restrict__ Q,
    double* faces[2*Dimensions]
) {
  assertion(false);
  for(int d=0; d<Dimensions; d++) {
    /**
     * d-loop over all dimensions except d. The vector k's entry d is set
     * to 0. We start with the left/bottom face, i.e. the one closer to the
     * coordinate system's origin.
     */
    dfore(k,numberOfVolumesPerAxisInPatch,d,0) {
      for (int i=0; i<haloSize; i++) {
        tarch::la::Vector<Dimensions,int> patchCell   = k + tarch::la::Vector<Dimensions,int>(haloSize);
        tarch::la::Vector<Dimensions,int> overlapCell = k;
        patchCell(d)   = i;
        overlapCell(d) = i; // this is correct

        int patchCellSerialised   = peano4::utils::dLinearised(patchCell,numberOfVolumesPerAxisInPatch+2*haloSize);
        int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);

        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }

        patchCell(d)   = i+numberOfVolumesPerAxisInPatch+haloSize;
        overlapCell(d) = i+haloSize;

        patchCellSerialised   = peano4::utils::dLinearised(patchCell,numberOfVolumesPerAxisInPatch+2*haloSize);
        overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,numberOfVolumesPerAxisInPatch,haloSize,d);
        for (int j=0; j<unknowns+auxiliaryVariables; j++) {
          faces[d+Dimensions][overlapCellSerialised*(unknowns+auxiliaryVariables)+j] =
            Q[patchCellSerialised*(unknowns+auxiliaryVariables)+j];
        }
      }
    }
  }
}
