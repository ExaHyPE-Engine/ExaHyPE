// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ApplySplit1DRiemannToPatchTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"
#include "tarch/tests/TestMacros.h"

exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest::ApplySplit1DRiemannToPatchTest():
  TestCase("exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest") {}

void exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest::run() { testMethod(testIterateGrid); }

void exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest::testIterateGrid() {
  double* numericalFluxL = ::tarch::allocateMemory<double>(5, tarch::MemoryLocation::Heap);
  double* numericalFluxR = ::tarch::allocateMemory<double>(5, tarch::MemoryLocation::Heap);
  numericalFluxL[0]      = 1.0;
  numericalFluxL[1]      = 2.0;
  numericalFluxL[2]      = 3.0;
  numericalFluxL[3]      = 4.0;
  numericalFluxL[4]      = 5.0;
  numericalFluxR[0]      = 6.0;
  numericalFluxR[1]      = 7.0;
  numericalFluxR[2]      = 8.0;
  numericalFluxR[3]      = 9.0;
  numericalFluxR[4]      = 10.0;

  constexpr int                               NumberOfVolumesPerAxisInPatch = 10;
  constexpr int                               NumberOfUnknowns              = 5;
  constexpr int                               NumberOfAuxiliaryVariables    = 0;
  constexpr double                            dt                            = 0.1;
  const tarch::la::Vector<Dimensions, double> volumeH                       = {0.01, 0.01};

  double* Qout = new double[500];
  double* Qcmp = new double[500];
  for (unsigned int i = 0; i < 500; ++i) {
    Qout[i] = 0.0;
    Qcmp[i] = 1.0 * i;
  }

  for (int y = 0; y < NumberOfVolumesPerAxisInPatch; y++) {
    for (int x = 0; x <= NumberOfVolumesPerAxisInPatch; x++) {
      const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxisInPatch;
      const int rightVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        if (x > 0) {
          Qout[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
          Qout[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
        if (x < NumberOfVolumesPerAxisInPatch) {
          Qout[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
          Qout[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
            [rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
      }
    }
  }

  for (int y = 0; y <= NumberOfVolumesPerAxisInPatch; y++) {
    for (int x = 0; x < NumberOfVolumesPerAxisInPatch; x++) {
      const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxisInPatch;
      const int upperVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        if (y > 0) {
          Qout[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
          Qout[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
            [lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
        if (y < NumberOfVolumesPerAxisInPatch) {
          Qout[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
          Qout[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
            [upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
        }
      }
    }
  }

  double* Qouttest = new double[500];
  for (unsigned int i = 0; i < 500; ++i) {
    Qouttest[i] = 0.0;
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int x = shift; x <= NumberOfVolumesPerAxisInPatch; x += 2) {
      for (int y = 0; y < NumberOfVolumesPerAxisInPatch; y++) {
        const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxisInPatch;
        const int rightVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (x > 0) {
            Qouttest[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
            Qouttest[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (x < NumberOfVolumesPerAxisInPatch) {
            Qouttest[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
            Qouttest[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int y = shift; y <= NumberOfVolumesPerAxisInPatch; y += 2) {
      for (int x = 0; x < NumberOfVolumesPerAxisInPatch; x++) {
        const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxisInPatch;
        const int upperVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (y > 0) {
            Qouttest[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
            Qouttest[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (y < NumberOfVolumesPerAxisInPatch) {
            Qouttest[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
            Qouttest[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  double* Qouttest2 = new double[500];
  for (unsigned int i = 0; i < 500; ++i) {
    Qouttest2[i] = 0.0;
  }

  for (int shift = 0; shift < 2; shift++) {
    for (int x = shift; x <= NumberOfVolumesPerAxisInPatch; x += 2) {
      for (int y = 0; y < NumberOfVolumesPerAxisInPatch; y++) {
        const int leftVoxelInImage  = x - 1 + y * NumberOfVolumesPerAxisInPatch;
        const int rightVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (x > 0) {
            Qouttest2[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
            Qouttest2[leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [leftVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (x < NumberOfVolumesPerAxisInPatch) {
            Qouttest2[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
            Qouttest2[rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [rightVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  // Iterate over other normal
  for (int shift = 0; shift < 2; shift++) {
    for (int y = shift; y <= NumberOfVolumesPerAxisInPatch; y += 2) {
      for (int x = 0; x < NumberOfVolumesPerAxisInPatch; x++) {
        const int lowerVoxelInImage = x + (y - 1) * NumberOfVolumesPerAxisInPatch;
        const int upperVoxelInImage = x + y * NumberOfVolumesPerAxisInPatch;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          if (y > 0) {
            Qouttest2[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] -= dt / volumeH(0) * numericalFluxL[unknown];
            Qouttest2[lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [lowerVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
          if (y < NumberOfVolumesPerAxisInPatch) {
            Qouttest2[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += dt / volumeH(0) * numericalFluxR[unknown];
            Qouttest2[upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown] += Qcmp
              [upperVoxelInImage * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + unknown];
          }
        }
      }
    }
  }

  // Elementwise comparison
  for (unsigned int i = 0; i < 500; ++i) {
    validateNumericalEquals(Qout[i], Qouttest[i]);
    validateNumericalEquals(Qouttest[i], Qouttest2[i]);
  }

  delete[] Qout;
  delete[] Qouttest;
  ::tarch::freeMemory(numericalFluxL, ::tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(numericalFluxR, ::tarch::MemoryLocation::Heap);
}
