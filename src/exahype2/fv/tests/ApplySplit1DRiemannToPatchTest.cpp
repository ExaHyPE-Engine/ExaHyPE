// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ApplySplit1DRiemannToPatchTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/tests/TestMacros.h"

exahype2::fv::tests::ApplySplit1DRiemannToPatchTest::ApplySplit1DRiemannToPatchTest() :
  TestCase("exahype2::fv::tests::ApplySplit1DRiemannToPatchTest") {}

void exahype2::fv::tests::ApplySplit1DRiemannToPatchTest::run() { testMethod(testSplitTraversalMatchesReference); }

/**
 * Verifies that red-black face traversal produces the same patch update as a
 * conventional traversal, including when the coordinate normals are grouped separately.
 */
void exahype2::fv::tests::ApplySplit1DRiemannToPatchTest::testSplitTraversalMatchesReference() {
  constexpr int    NumberOfVolumesPerAxis     = 10;
  constexpr int    NumberOfUnknowns           = 5;
  constexpr int    NumberOfAuxiliaryVariables = 0;
  constexpr int    NumberOfPatchValues        = NumberOfVolumesPerAxis * NumberOfVolumesPerAxis * NumberOfUnknowns;
  constexpr double TimeStepSize               = 0.1;
  const tarch::la::Vector<DIMENSIONS, double> volumeSize = {0.01, 0.01};

  double* leftNumericalFlux  = tarch::allocateMemory<double>(NumberOfUnknowns, tarch::MemoryLocation::Heap);
  double* rightNumericalFlux = tarch::allocateMemory<double>(NumberOfUnknowns, tarch::MemoryLocation::Heap);
  leftNumericalFlux[0]       = 1.0;
  leftNumericalFlux[1]       = 2.0;
  leftNumericalFlux[2]       = 3.0;
  leftNumericalFlux[3]       = 4.0;
  leftNumericalFlux[4]       = 5.0;
  rightNumericalFlux[0]      = 6.0;
  rightNumericalFlux[1]      = 7.0;
  rightNumericalFlux[2]      = 8.0;
  rightNumericalFlux[3]      = 9.0;
  rightNumericalFlux[4]      = 10.0;

  double* referenceUpdate = new double[NumberOfPatchValues];
  double* initialValues   = new double[NumberOfPatchValues];
  for (int valueIndex = 0; valueIndex < NumberOfPatchValues; ++valueIndex) {
    referenceUpdate[valueIndex] = 0.0;
    initialValues[valueIndex]   = 1.0 * valueIndex;
  }

  for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
    for (int x = 0; x <= NumberOfVolumesPerAxis; x++) {
      const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxis;
      const int rightVoxelInImage = x + y * NumberOfVolumesPerAxis;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        if (x > 0) {
          referenceUpdate[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
            TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
          referenceUpdate[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            initialValues[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
        if (x < NumberOfVolumesPerAxis) {
          referenceUpdate[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
          referenceUpdate[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            initialValues[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
      }
    }
  }

  for (int y = 0; y <= NumberOfVolumesPerAxis; y++) {
    for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
      const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxis;
      const int upperVoxelInImage = x + y * NumberOfVolumesPerAxis;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        if (y > 0) {
          referenceUpdate[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
            TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
          referenceUpdate[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            initialValues[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
        if (y < NumberOfVolumesPerAxis) {
          referenceUpdate[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
          referenceUpdate[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
            initialValues[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
      }
    }
  }

  double* splitUpdate = new double[NumberOfPatchValues];
  for (int valueIndex = 0; valueIndex < NumberOfPatchValues; ++valueIndex) {
    splitUpdate[valueIndex] = 0.0;
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int x = shift; x <= NumberOfVolumesPerAxis; x += 2) {
      for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
        const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxis;
        const int rightVoxelInImage = x + y * NumberOfVolumesPerAxis;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (x > 0) {
            splitUpdate[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
              TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
            splitUpdate[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (x < NumberOfVolumesPerAxis) {
            splitUpdate[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
            splitUpdate[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int y = shift; y <= NumberOfVolumesPerAxis; y += 2) {
      for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
        const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxis;
        const int upperVoxelInImage = x + y * NumberOfVolumesPerAxis;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (y > 0) {
            splitUpdate[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
              TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
            splitUpdate[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (y < NumberOfVolumesPerAxis) {
            splitUpdate[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
            splitUpdate[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  double* splitUpdateWithGroupedNormals = new double[NumberOfPatchValues];
  for (int valueIndex = 0; valueIndex < NumberOfPatchValues; ++valueIndex) {
    splitUpdateWithGroupedNormals[valueIndex] = 0.0;
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int x = shift; x <= NumberOfVolumesPerAxis; x += 2) {
      for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
        const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxis;
        const int rightVoxelInImage = x + y * NumberOfVolumesPerAxis;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (x > 0) {
            splitUpdateWithGroupedNormals
              [leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
              TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
            splitUpdateWithGroupedNormals
              [leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (x < NumberOfVolumesPerAxis) {
            splitUpdateWithGroupedNormals
              [rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
            splitUpdateWithGroupedNormals
              [rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int y = shift; y <= NumberOfVolumesPerAxis; y += 2) {
      for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
        const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxis;
        const int upperVoxelInImage = x + y * NumberOfVolumesPerAxis;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (y > 0) {
            splitUpdateWithGroupedNormals
              [lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -=
              TimeStepSize / volumeSize(0) * leftNumericalFlux[unknown];
            splitUpdateWithGroupedNormals
              [lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (y < NumberOfVolumesPerAxis) {
            splitUpdateWithGroupedNormals
              [upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              TimeStepSize / volumeSize(0) * rightNumericalFlux[unknown];
            splitUpdateWithGroupedNormals
              [upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] +=
              initialValues[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  for (int valueIndex = 0; valueIndex < NumberOfPatchValues; ++valueIndex) {
    validateNumericalEquals(referenceUpdate[valueIndex], splitUpdate[valueIndex]);
    validateNumericalEquals(splitUpdate[valueIndex], splitUpdateWithGroupedNormals[valueIndex]);
  }

  delete[] referenceUpdate;
  delete[] initialValues;
  delete[] splitUpdate;
  delete[] splitUpdateWithGroupedNormals;
  tarch::freeMemory(leftNumericalFlux, tarch::MemoryLocation::Heap);
  tarch::freeMemory(rightNumericalFlux, tarch::MemoryLocation::Heap);
}
