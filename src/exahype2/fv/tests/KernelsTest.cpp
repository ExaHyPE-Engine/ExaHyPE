// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsTest.h"

#include "exahype2/fv/Kernels.h"
#include "exahype2/fv/tests/HaloProfileTestData.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"

namespace {
  using Setup = exahype2::fv::tests::HaloProfileSetup;

  /**
   * Solver stand-in with the patch layout the transfer reads.
   */
  struct PatchLayout final {
    static constexpr int PatchSize                  = Setup::DofsPerAxis;
    static constexpr int NumberOfUnknowns           = Setup::NumberOfData;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  using CellPool = tarch::accelerator::ChunkedMemoryPool<double>;
  using Transfer = exahype2::fv::PatchTransfer<PatchLayout, double, double>;

  void fillPatch(double* const values, const int slot) {
    for (int volume = 0; volume < Setup::VolumesPerCell; volume++) {
      for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
        values[volume * Setup::NumberOfData + unknown] = 100.0 * slot + 10.0 * volume + unknown;
      }
    }
  }
} // namespace

exahype2::fv::tests::KernelsTest::KernelsTest() :
  tarch::tests::TestCase("exahype2::fv::tests::KernelsTest") {}

void
exahype2::fv::tests::KernelsTest::run() {
  testMethod(testCopyKeepsEveryValue);
  testMethod(testInterpolationFromTheParentCopiesTheCoveringVolume);
  testMethod(testRestrictionFromTheChildrenAveragesTheCoveredVolumes);
}

void
exahype2::fv::tests::KernelsTest::testCopyKeepsEveryValue() {
  CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
  pool.reserve(1);
  fillPatch(pool.getChunk(0), 0);

  double target[Setup::ValuesPerCell];
  for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
    Transfer{std::as_const(pool).getView()}.copy(0, target, volume);
  }

  for (int value = 0; value < Setup::ValuesPerCell; value++) {
    validateEqualsWithParams1(target[value], pool.getChunk(0)[value], value);
  }
}

void
exahype2::fv::tests::KernelsTest::testInterpolationFromTheParentCopiesTheCoveringVolume() {
  CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
  pool.reserve(1);
  fillPatch(pool.getChunk(0), 0);

  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    double target[Setup::ValuesPerCell];
    for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
      Transfer{std::as_const(pool).getView()}.interpolateFromParent(0, target, child, volume, nullptr);
    }

    const auto childPosition = peano4::dDelinearised(child, Setup::SubdivisionFactor);
    peano4::dForEach(Setup::DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& fineVolume) {
      tarch::la::Vector<DIMENSIONS, int> coarseVolume;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        coarseVolume(axis) = (fineVolume(axis) + Setup::DofsPerAxis * childPosition(axis)) / Setup::SubdivisionFactor;
      }
      const int fineIndex   = peano4::dLinearised(fineVolume, Setup::DofsPerAxis) * Setup::NumberOfData;
      const int coarseIndex = peano4::dLinearised(coarseVolume, Setup::DofsPerAxis) * Setup::NumberOfData;
      for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
        validateEqualsWithParams2(target[fineIndex + unknown], pool.getChunk(0)[coarseIndex + unknown], child, unknown);
      }
    });
  }
}

void
exahype2::fv::tests::KernelsTest::testRestrictionFromTheChildrenAveragesTheCoveredVolumes() {
  CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
  pool.reserve(Setup::ChildrenPerCell);
  int childSlots[Setup::ChildrenPerCell];
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    fillPatch(pool.getChunk(child), child);
    childSlots[child] = child;
  }

  double target[Setup::ValuesPerCell];
  for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
    Transfer{std::as_const(pool).getView()}.restrictFromChildren(childSlots, target, volume, nullptr);
  }

  constexpr int VolumesPerCoarseVolume = peano4::power(Setup::SubdivisionFactor, DIMENSIONS);
  peano4::dForEach(Setup::DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& coarseVolume) {
    const int coarseIndex = peano4::dLinearised(coarseVolume, Setup::DofsPerAxis) * Setup::NumberOfData;
    for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
      double sum = 0.0;
      peano4::dForEach(Setup::SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& fineOffset) {
        tarch::la::Vector<DIMENSIONS, int> childPosition;
        tarch::la::Vector<DIMENSIONS, int> fineVolume;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const int globalFineVolume = Setup::SubdivisionFactor * coarseVolume(axis) + fineOffset(axis);
          childPosition(axis)        = globalFineVolume / Setup::DofsPerAxis;
          fineVolume(axis)           = globalFineVolume % Setup::DofsPerAxis;
        }
        const double* const child = pool.getChunk(peano4::dLinearised(childPosition, Setup::SubdivisionFactor));
        sum += child[peano4::dLinearised(fineVolume, Setup::DofsPerAxis) * Setup::NumberOfData + unknown];
      });
      validateNumericalEqualsWithParams2(
        target[coarseIndex + unknown],
        sum / VolumesPerCoarseVolume,
        coarseVolume,
        unknown
      );
    }
  });
}
