// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchKernelsTest.h"

#include "peano4/Loop.h"
#include "exahype2/Enumeration.h"
#include "exahype2/ProjectPatch.h"
#include "exahype2/ReconstructPatch.h"

namespace {
  // Distinct value for one unknown of one patch volume, so a misrouted copy
  // shows up as a mismatch.
  double patchValue(const int serialisedVolume, const int unknown) { return 1000.0 * serialisedVolume + unknown + 1; }

  // Distinct value for one unknown of one face overlap voxel, kept apart from
  // the patch values so a gather from the wrong source is visible.
  double faceValue(const int face, const int serialisedVoxel, const int unknown) {
    return -1.0 - (100.0 * face + 7.0 * serialisedVoxel + unknown);
  }

  template <int NumberOfDofsPerAxis>
  int numberOfPatchVolumes() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      product *= NumberOfDofsPerAxis;
    }
    return product;
  }

  template <int NumberOfDofsPerAxis, int Overlap>
  int faceOverlapVoxels() {
    int product = 2 * Overlap;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= NumberOfDofsPerAxis;
    }
    return product;
  }
} // namespace

exahype2::tests::PatchKernelsTest::PatchKernelsTest() :
  tarch::tests::TestCase("exahype2::tests::PatchKernelsTest") {}

void exahype2::tests::PatchKernelsTest::run() {
  testMethod(testProjectPatchFillsFaces);
  testMethod(testReconstructPatchGathersInteriorAndHalo);
}

/**
 * Verifies that projection maps each boundary volume to its matching face overlap.
 */
void exahype2::tests::PatchKernelsTest::testProjectPatchFillsFaces() {
  constexpr int NumberOfDofsPerAxis = 4;
  constexpr int Overlap             = 2;
  constexpr int NumberOfUnknowns    = 2;

  const int patchVolumes = numberOfPatchVolumes<NumberOfDofsPerAxis>();
  const int faceVoxels   = faceOverlapVoxels<NumberOfDofsPerAxis, Overlap>();

  std::vector<double> patch(patchVolumes * NumberOfUnknowns);
  for (int volume = 0; volume < patchVolumes; volume++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      patch[volume * NumberOfUnknowns + unknown] = patchValue(volume, unknown);
    }
  }

  std::vector<std::vector<double>> faceStorage(TWO_TIMES_D, std::vector<double>(faceVoxels * NumberOfUnknowns, 0.0));
  double*                          faces[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
    faces[face] = faceStorage[face].data();
  }

  exahype2::projectPatch<NumberOfDofsPerAxis, Overlap, NumberOfUnknowns, double>(patch.data(), faces);

  tarch::la::Vector<DIMENSIONS, int> overlapRange(NumberOfDofsPerAxis);
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    overlapRange(normal) = 2 * Overlap;
    peano4::dForEach(overlapRange, [&](const tarch::la::Vector<DIMENSIONS, int>& overlapVoxel) {
      const int overlapIndex =
        exahype2::serialiseVoxelIndexInOverlap(overlapVoxel, NumberOfDofsPerAxis, Overlap, normal);

      // The lower face keeps this patch's boundary in its upper overlap half,
      // the upper face keeps it in its lower half.
      if (overlapVoxel(normal) >= Overlap) {
        tarch::la::Vector<DIMENSIONS, int> boundaryVolume = overlapVoxel;
        boundaryVolume(normal)                            = overlapVoxel(normal) - Overlap;
        const int boundaryIndex                           = peano4::dLinearised(boundaryVolume, NumberOfDofsPerAxis);
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams2(
            faceStorage[normal][overlapIndex * NumberOfUnknowns + unknown],
            patchValue(boundaryIndex, unknown),
            overlapVoxel,
            normal
          );
        }
      } else {
        tarch::la::Vector<DIMENSIONS, int> boundaryVolume = overlapVoxel;
        boundaryVolume(normal)                            = overlapVoxel(normal) + NumberOfDofsPerAxis - Overlap;
        const int boundaryIndex                           = peano4::dLinearised(boundaryVolume, NumberOfDofsPerAxis);
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams2(
            faceStorage[normal + DIMENSIONS][overlapIndex * NumberOfUnknowns + unknown],
            patchValue(boundaryIndex, unknown),
            overlapVoxel,
            normal
          );
        }
      }
    });
    overlapRange(normal) = NumberOfDofsPerAxis;
  }
}

/**
 * Verifies that reconstruction copies the patch interior and each unambiguous halo volume.
 */
void exahype2::tests::PatchKernelsTest::testReconstructPatchGathersInteriorAndHalo() {
  constexpr int NumberOfDofsPerAxis  = 4;
  constexpr int Overlap              = 2;
  constexpr int NumberOfUnknowns     = 2;
  constexpr int ReconstructedPerAxis = NumberOfDofsPerAxis + 2 * Overlap;

  const int patchVolumes         = numberOfPatchVolumes<NumberOfDofsPerAxis>();
  const int faceVoxels           = faceOverlapVoxels<NumberOfDofsPerAxis, Overlap>();
  int       reconstructedVolumes = 1;
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    reconstructedVolumes *= ReconstructedPerAxis;
  }

  std::vector<double> patch(patchVolumes * NumberOfUnknowns);
  for (int volume = 0; volume < patchVolumes; volume++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      patch[volume * NumberOfUnknowns + unknown] = patchValue(volume, unknown);
    }
  }

  std::vector<std::vector<double>> faceStorage(TWO_TIMES_D, std::vector<double>(faceVoxels * NumberOfUnknowns, 0.0));
  double*                          faces[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
    for (int voxel = 0; voxel < faceVoxels; voxel++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        faceStorage[face][voxel * NumberOfUnknowns + unknown] = faceValue(face, voxel, unknown);
      }
    }
    faces[face] = faceStorage[face].data();
  }

  std::vector<double> reconstructed(reconstructedVolumes * NumberOfUnknowns, 0.0);
  exahype2::reconstructPatch<NumberOfDofsPerAxis, Overlap, NumberOfUnknowns, double>(
    patch.data(),
    faces,
    reconstructed.data()
  );

  const tarch::la::Vector<DIMENSIONS, int> overlapShift(Overlap);

  tarch::la::Vector<DIMENSIONS, int> patchRange(NumberOfDofsPerAxis);
  peano4::dForEach(patchRange, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const int                                patchIndex          = peano4::dLinearised(volume, NumberOfDofsPerAxis);
    const tarch::la::Vector<DIMENSIONS, int> reconstructedVolume = volume + overlapShift;
    const int reconstructedIndex = peano4::dLinearised(reconstructedVolume, ReconstructedPerAxis);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams1(
        reconstructed[reconstructedIndex * NumberOfUnknowns + unknown],
        patchValue(patchIndex, unknown),
        volume
      );
    }
  });

  tarch::la::Vector<DIMENSIONS, int> reconstructedRange(ReconstructedPerAxis);
  peano4::dForEach(reconstructedRange, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
    int haloDirections = 0;
    int normal         = -1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      const bool insideAlongAxis =
        haloVolume(dimension) >= Overlap and haloVolume(dimension) < NumberOfDofsPerAxis + Overlap;
      if (not insideAlongAxis) {
        haloDirections++;
        normal = dimension;
      }
    }
    // Corner halo voxels are written once per direction, so only the last write
    // survives; the single-direction voxels have one unambiguous source.
    if (haloDirections != 1) {
      return;
    }

    const int                          reconstructedIndex = peano4::dLinearised(haloVolume, ReconstructedPerAxis);
    tarch::la::Vector<DIMENSIONS, int> overlapVoxel       = haloVolume - overlapShift;
    int                                face               = normal;
    if (haloVolume(normal) < Overlap) {
      overlapVoxel(normal) = haloVolume(normal);
    } else {
      overlapVoxel(normal) = haloVolume(normal) - NumberOfDofsPerAxis;
      face                 = normal + DIMENSIONS;
    }
    const int overlapIndex =
      exahype2::serialiseVoxelIndexInOverlap(overlapVoxel, NumberOfDofsPerAxis, Overlap, normal);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        reconstructed[reconstructedIndex * NumberOfUnknowns + unknown],
        faceValue(face, overlapIndex, unknown),
        haloVolume,
        normal
      );
    }
  });
}
