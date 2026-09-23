// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceKernelFixtures.h"

void exahype2::fv::tests::DeviceDeleter::operator()(void* const data) const {
  tarch::freeMemory(data, tarch::MemoryLocation::Device);
}

/**
 * Returns count values that differ per entry, so an offset that is off by any
 * amount changes the result.
 */
std::vector<double> exahype2::fv::tests::makeRamp(
  const std::size_t count,
  const double      offset,
  const double      increment
) {
  std::vector<double> result(count);
  for (std::size_t entry = 0; entry < count; entry++) {
    result[entry] = offset + increment * static_cast<double>(entry);
  }
  return result;
}

exahype2::fv::tests::PatchChain::PatchChain() {
  for (int patch = 0; patch < DeviceKernelSetup::NumberOfPatches; patch++) {
    const int firstCoordinate = patch * DIMENSIONS;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      patchSizes[firstCoordinate + axis] = (0.3 + 0.1 * axis) * (1.0 + 0.05 * patch);
    }
    patchCentres[firstCoordinate] = (patch + 0.5) * patchSizes[firstCoordinate];
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      patchCentres[firstCoordinate + axis] = 0.5 * patchSizes[firstCoordinate + axis];
    }
  }

  int nextFaceIndex = DeviceKernelSetup::NumberOfPatches + 1;
  for (int patch = 0; patch < DeviceKernelSetup::NumberOfPatches; patch++) {
    const int firstFace                 = patch * peano4::FacesPerCell;
    faceIndices[firstFace]              = patch;
    faceIndices[firstFace + DIMENSIONS] = patch + 1;
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      faceIndices[firstFace + axis]              = nextFaceIndex++;
      faceIndices[firstFace + axis + DIMENSIONS] = nextFaceIndex++;
    }
  }
  numberOfUniqueFaces = nextFaceIndex;

  std::vector<bool> alreadyListed(numberOfUniqueFaces, false);
  for (int patch = 0; patch < DeviceKernelSetup::NumberOfPatches; patch++) {
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const std::uint64_t faceIndex = faceIndices[patch * peano4::FacesPerCell + faceNumber];
      if (not alreadyListed[faceIndex]) {
        alreadyListed[faceIndex] = true;
        workFaceIndices.push_back(faceIndex);
        workFaceNumbers.push_back(static_cast<std::uint8_t>(faceNumber));
        workPatchIndices.push_back(patch);
      }
    }
  }
}

int exahype2::fv::tests::PatchChain::numberOfWorkFaces() const { return static_cast<int>(workFaceIndices.size()); }

int exahype2::fv::tests::PatchChain::faceOverlapEntries(const int haloSize) const {
  return numberOfUniqueFaces * DeviceKernelSetup::faceOverlapCardinality(haloSize);
}
