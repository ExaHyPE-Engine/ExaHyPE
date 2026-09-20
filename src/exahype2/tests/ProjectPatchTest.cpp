// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchTest.h"

#include "exahype2/Enumeration.h"
#include "exahype2/ProjectPatch.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"

namespace {
  // Returns a distinct value for one unknown of one patch volume, so a misrouted
  // copy shows up as a mismatch.
  double patchValue(const int serialisedVolume, const int unknown) { return 1000.0 * serialisedVolume + unknown + 1; }

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

exahype2::tests::ProjectPatchTest::ProjectPatchTest() :
  tarch::tests::TestCase("exahype2::tests::ProjectPatchTest") {}

void exahype2::tests::ProjectPatchTest::run() { testMethod(testProjectPatchFillsFaces); }

/**
 * Verifies that projection maps each boundary volume to its matching face overlap.
 */
void exahype2::tests::ProjectPatchTest::testProjectPatchFillsFaces() {
  constexpr int NumberOfDofsPerAxis = 4;
  constexpr int Overlap             = 2;
  constexpr int NumberOfUnknowns    = 2;

  const int patchVolumes = numberOfPatchVolumes<NumberOfDofsPerAxis>();
  const int faceVoxels   = faceOverlapVoxels<NumberOfDofsPerAxis, Overlap>();

  std::vector<double> patch(static_cast<std::size_t>(patchVolumes * NumberOfUnknowns));
  for (int volume = 0; volume < patchVolumes; volume++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      patch[volume * NumberOfUnknowns + unknown] = patchValue(volume, unknown);
    }
  }

  std::vector<std::vector<double>> faceStorage(
    peano4::FacesPerCell,
    std::vector<double>(static_cast<std::size_t>(faceVoxels * NumberOfUnknowns), 0.0)
  );
  double* faces[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
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
