// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryTest.h"

#include "exahype2/fv/HandleBoundary.h"
#include "toolbox/blockstructured/Enumeration.h"

namespace {
  // Copies the inner state into the halo, the discrete equivalent of a
  // homogeneous Neumann (outflow) condition.
  template <int NumberOfData>
  struct CopyBoundarySolver final {
    static void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        outsideState[entry] = insideState[entry];
      }
    }
  };

  // Writes inner value plus one into the halo. A doubly written halo volume
  // would therefore exceed one and expose an index collision.
  template <int NumberOfData>
  struct IncrementBoundarySolver final {
    static void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        outsideState[entry] = insideState[entry] + 1.0;
      }
    }
  };

  constexpr int NumberOfVolumesPerAxis     = 4;
  constexpr int HaloSize                   = 1;
  constexpr int MusclHaloSize              = 2;
  constexpr int NumberOfUnknowns           = 3;
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  constexpr int numberOfTangentialVolumes() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= NumberOfVolumesPerAxis;
    }
    return product;
  }

  // A face overlap holds 2 * HaloSize layers along the normal and
  // NumberOfVolumesPerAxis layers along every other axis.
  constexpr int faceOverlapStorageSize(const int haloSize) {
    return 2 * haloSize * numberOfTangentialVolumes() * NumberOfData;
  }
} // namespace

exahype2::fv::tests::HandleBoundaryTest::HandleBoundaryTest() :
  tarch::tests::TestCase("exahype2::fv::tests::HandleBoundaryTest") {}

void exahype2::fv::tests::HandleBoundaryTest::run() {
  testMethod(testCopyPreservesUniformState);
  testMethod(testEachGhostLayerUsesMatchingInteriorLayer);
  testMethod(testEachOutsideVolumeIsWrittenOnce);
}

void exahype2::fv::tests::HandleBoundaryTest::testCopyPreservesUniformState() {
  CopyBoundarySolver<NumberOfData>            solver;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
  const double                                uniformValue = 1.23;

  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    std::vector<double> faceValues(faceOverlapStorageSize(HaloSize), uniformValue);

    exahype2::fv::handleBoundary<
      CopyBoundarySolver<NumberOfData>,
      NumberOfVolumesPerAxis,
      HaloSize,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double>(solver, patchCentre, patchSize, 0.0, faceNumber, faceValues.data());

    for (const double entry : faceValues) {
      validateNumericalEquals(entry, uniformValue);
    }
  }
}

void exahype2::fv::tests::HandleBoundaryTest::testEachGhostLayerUsesMatchingInteriorLayer() {
  CopyBoundarySolver<NumberOfData>            solver;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);

  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    std::vector<double> faceValues(faceOverlapStorageSize(MusclHaloSize), -1.0);
    const int           normal = faceNumber % DIMENSIONS;

    for (int tangentialVolume = 0; tangentialVolume < numberOfTangentialVolumes(); tangentialVolume++) {
      tarch::la::Vector<DIMENSIONS, int> overlapVolume;
      int                                remainder = tangentialVolume;
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        if (dimension == normal) {
          overlapVolume(dimension) = 0;
        } else {
          overlapVolume(dimension) = remainder % NumberOfVolumesPerAxis;
          remainder /= NumberOfVolumesPerAxis;
        }
      }

      for (int layer = 0; layer < MusclHaloSize; layer++) {
        overlapVolume(normal)   = faceNumber < DIMENSIONS ? MusclHaloSize + layer : MusclHaloSize - 1 - layer;
        const auto insideVolume = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          overlapVolume,
          NumberOfVolumesPerAxis,
          MusclHaloSize,
          normal
        );
        for (int entry = 0; entry < NumberOfData; entry++) {
          faceValues[insideVolume * NumberOfData + entry] =
            static_cast<double>(100 * tangentialVolume + 10 * layer + entry);
        }
      }
    }

    exahype2::fv::handleBoundary<
      CopyBoundarySolver<NumberOfData>,
      NumberOfVolumesPerAxis,
      MusclHaloSize,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double>(solver, patchCentre, patchSize, 0.0, faceNumber, faceValues.data());

    for (int tangentialVolume = 0; tangentialVolume < numberOfTangentialVolumes(); tangentialVolume++) {
      tarch::la::Vector<DIMENSIONS, int> overlapVolume;
      int                                remainder = tangentialVolume;
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        if (dimension == normal) {
          overlapVolume(dimension) = 0;
        } else {
          overlapVolume(dimension) = remainder % NumberOfVolumesPerAxis;
          remainder /= NumberOfVolumesPerAxis;
        }
      }

      for (int layer = 0; layer < MusclHaloSize; layer++) {
        overlapVolume(normal)    = faceNumber < DIMENSIONS ? MusclHaloSize - 1 - layer : MusclHaloSize + layer;
        const auto outsideVolume = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          overlapVolume,
          NumberOfVolumesPerAxis,
          MusclHaloSize,
          normal
        );
        for (int entry = 0; entry < NumberOfData; entry++) {
          const auto expectedValue = static_cast<double>(100 * tangentialVolume + 10 * layer + entry);
          validateNumericalEquals(faceValues[outsideVolume * NumberOfData + entry], expectedValue);
        }
      }
    }
  }
}

void exahype2::fv::tests::HandleBoundaryTest::testEachOutsideVolumeIsWrittenOnce() {
  IncrementBoundarySolver<NumberOfData>       solver;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);

  const int expectedWrittenEntries = HaloSize * numberOfTangentialVolumes() * NumberOfData;

  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    std::vector<double> faceValues(faceOverlapStorageSize(HaloSize), 0.0);

    exahype2::fv::handleBoundary<
      IncrementBoundarySolver<NumberOfData>,
      NumberOfVolumesPerAxis,
      HaloSize,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double>(solver, patchCentre, patchSize, 0.0, faceNumber, faceValues.data());

    int    numberOfWrittenEntries = 0;
    double maximumEntry           = 0.0;
    for (const double entry : faceValues) {
      if (entry > 0.5) {
        numberOfWrittenEntries++;
      }
      if (entry > maximumEntry) {
        maximumEntry = entry;
      }
    }

    validateEquals(numberOfWrittenEntries, expectedWrittenEntries);
    validateNumericalEquals(maximumEntry, 1.0);
  }
}
