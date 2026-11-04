// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/Kernels.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/fv/HaloProfileTestData.h"

namespace {
  using Setup = exahype2::fv::unittests::HaloProfileSetup;

  /**
   * A solver stand-in with the patch layout that the transfer reads.
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

UNIT_TEST("A cell that survives the regrid keeps every value of its old slot") {
  GIVEN("A pool with one filled patch") {
    CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
    pool.reserve(1);
    fillPatch(pool.getChunk(0), 0);

    WHEN("The transfer copies the patch of the slot") {
      double target[Setup::ValuesPerCell];
      for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
        Transfer{std::as_const(pool).getView()}.copy(0, target, volume);
      }

      THEN("The copy contains every value of the slot") {
        for (int value = 0; value < Setup::ValuesPerCell; value++) {
          VALIDATE_EQUALS(target[value], pool.getChunk(0)[value], value);
        }
      }
    }
  }
}

UNIT_TEST("Every volume of a new child takes the value of the parent volume that covers it") {
  GIVEN("A pool with one filled parent patch") {
    CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
    pool.reserve(1);
    fillPatch(pool.getChunk(0), 0);

    WHEN("The transfer interpolates every child from the parent") {
      std::vector<std::vector<double>> children(Setup::ChildrenPerCell, std::vector<double>(Setup::ValuesPerCell));
      for (int child = 0; child < Setup::ChildrenPerCell; child++) {
        for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
          Transfer{std::as_const(pool).getView()}
            .interpolateFromParent(0, children[child].data(), child, volume, nullptr);
        }
      }

      THEN("Every fine volume of every child equals the parent volume that covers it") {
        for (int child = 0; child < Setup::ChildrenPerCell; child++) {
          const auto childPosition = peano4::dDelinearised(child, Setup::SubdivisionFactor);
          peano4::dForEach(Setup::DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& fineVolume) {
            tarch::la::Vector<DIMENSIONS, int> coarseVolume;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              coarseVolume(axis) =
                (fineVolume(axis) + Setup::DofsPerAxis * childPosition(axis)) / Setup::SubdivisionFactor;
            }
            const int fineIndex   = peano4::dLinearised(fineVolume, Setup::DofsPerAxis) * Setup::NumberOfData;
            const int coarseIndex = peano4::dLinearised(coarseVolume, Setup::DofsPerAxis) * Setup::NumberOfData;
            for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
              VALIDATE_EQUALS(
                children[child][fineIndex + unknown],
                pool.getChunk(0)[coarseIndex + unknown],
                child,
                unknown
              );
            }
          });
        }
      }
    }
  }
}

UNIT_TEST("Every volume of a coarsened parent takes the mean of the child volumes it covers") {
  GIVEN("A pool with one filled patch per child and the slots of the children") {
    CellPool pool(Setup::ValuesPerCell, tarch::MemoryLocation::Heap);
    pool.reserve(Setup::ChildrenPerCell);
    int childSlots[Setup::ChildrenPerCell];
    for (int child = 0; child < Setup::ChildrenPerCell; child++) {
      fillPatch(pool.getChunk(child), child);
      childSlots[child] = child;
    }

    WHEN("The transfer restricts the children onto the parent") {
      double target[Setup::ValuesPerCell];
      for (int volume = 0; volume < Transfer::WorkItemsPerChunk; volume++) {
        Transfer{std::as_const(pool).getView()}.restrictFromChildren(childSlots, target, volume, nullptr);
      }

      THEN("Every coarse volume equals the mean of the fine volumes it covers") {
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
            VALIDATE_NUMERICAL_EQUALS(
              target[coarseIndex + unknown],
              sum / VolumesPerCoarseVolume,
              coarseVolume,
              unknown
            );
          }
        });
      }
    }
  }
}
