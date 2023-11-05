// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CopyPatchTest.h"

#include <vector>

#include "peano4/utils/Loop.h"
#include "tarch/tests/TestMacros.h"

exahype2::fv::rusanov::tests::CopyPatchTest::CopyPatchTest():
  TestCase("exahype2::fv::rusanov::tests::CopyPatchTest") {}

void exahype2::fv::rusanov::tests::CopyPatchTest::run() { testMethod(testCopyPatch); }

void exahype2::fv::rusanov::tests::CopyPatchTest::testCopyPatch() {
  constexpr int    NumberOfUnknowns              = 5;
  constexpr int    NumberOfAuxiliaryVariables    = 2;
  constexpr int    NumberOfVolumesPerAxisInPatch = 17;
  constexpr int    HaloSize                      = 1;
  double*          QinWithHalo                   = nullptr;
  double*          QOutWithoutHalo               = nullptr;
  std::vector<int> source{}, dest{}, source2{}, dest2{};

  dfor(k, NumberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions, int> tmpSource                = k + tarch::la::Vector<Dimensions, int>(HaloSize);
    int                                tmpSourceSerialised      = peano4::utils::dLinearised(tmpSource, NumberOfVolumesPerAxisInPatch + HaloSize * 2);
    int                                tmpDestinationSerialised = peano4::utils::dLinearised(k, NumberOfVolumesPerAxisInPatch);
    source.push_back(tmpSourceSerialised);
    dest.push_back(tmpDestinationSerialised);
  }

#if Dimensions == 2
  constexpr int SourceSerialised = NumberOfVolumesPerAxisInPatch + 3 * HaloSize;
  constexpr int Offset           = NumberOfVolumesPerAxisInPatch;
  for (int y = 0; y < NumberOfVolumesPerAxisInPatch; y++) {
    for (int x = 0; x < NumberOfVolumesPerAxisInPatch; x++) {
      source2.push_back((y + 1) * (Offset + 3 * HaloSize) + x - y);
      dest2.push_back(y * Offset + x);
    }
  }
#else
  constexpr int Offset  = NumberOfVolumesPerAxisInPatch;
  constexpr int Offset2 = (NumberOfVolumesPerAxisInPatch + HaloSize * 2) * (NumberOfVolumesPerAxisInPatch + HaloSize * 2) + NumberOfVolumesPerAxisInPatch + HaloSize * 2 + HaloSize;
  constexpr int Offset3 = NumberOfVolumesPerAxisInPatch + HaloSize * 2;
  for (int z = 0; z < NumberOfVolumesPerAxisInPatch; z++) {
    for (int y = 0; y < NumberOfVolumesPerAxisInPatch; y++) {
      for (int x = 0; x < NumberOfVolumesPerAxisInPatch; x++) {
        int mydest = z * Offset * Offset + y * Offset + x;
        int mysrc  = z * Offset3 * Offset3 + y * Offset3 + x + Offset2;
        source2.push_back(mysrc);
        dest2.push_back(mydest);
      }
    }
  }
#endif

  for (std::size_t idx = 0; idx < source.size(); idx++) {
    validateEquals(source[idx], source2[idx]);
  }

  for (std::size_t idx = 0; idx < dest.size(); idx++) {
    validateEquals(dest[idx], dest2[idx]);
  }
}
