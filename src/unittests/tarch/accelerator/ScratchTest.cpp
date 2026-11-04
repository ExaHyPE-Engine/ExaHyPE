// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Scratch.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"

#include <omp.h>

namespace {
  TARGET_DECLARE
  constexpr int ChunkSize = 4;

  /**
   * Counts a work item whose scratch chunk lies outside memory or loses a value before the work
   * item reads it back.
   */
  struct CountScratchViolation final {
    int* memory{nullptr};
    int  chunks{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int workItem, int& accumulator) const {
      const int chunk = tarch::accelerator::getScratchChunk(workItem);
      if (chunk < 0 or chunk >= chunks) {
        accumulator++;
        return;
      }
      int* const own = memory + chunk * ChunkSize;
      for (int entry = 0; entry < ChunkSize; entry++) {
        own[entry] = workItem + entry;
      }
      for (int entry = 0; entry < ChunkSize; entry++) {
        if (own[entry] != workItem + entry) {
          accumulator++;
        }
      }
    }
  };
  TARGET_DECLARE_END

  int getNumberOfScratchViolations(const int workItems, const int chunks, int* const memory) {
    int violations = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(workItems),
      CountScratchViolation{memory, chunks},
      tarch::accelerator::Sum<int>{violations}
    );
    return violations;
  }
} // namespace

UNIT_TEST("Every work item of a dispatch gets a chunk below the chunk count and reads back what it writes into it") {
  GIVEN("A dispatch of 1000 work items") {
    constexpr int WorkItems = 1000;

    WHEN("getNumberOfScratchChunks() returns the chunk counts of this dispatch and of an empty dispatch") {
      const int chunks       = tarch::accelerator::getNumberOfScratchChunks(WorkItems);
      const int chunksOfNone = tarch::accelerator::getNumberOfScratchChunks(0);

      THEN("The dispatch receives at least one chunk and the empty dispatch zero chunks or the same count") {
        VALIDATE(chunks >= 1);
        VALIDATE(chunksOfNone == 0 or chunksOfNone == chunks);
      }

      AND_WHEN("Every work item writes into its chunk of a device scratch buffer and reads the chunk back") {
        tarch::accelerator::Buffer<int> scratch{tarch::MemoryLocation::Device};
        scratch.allocate(static_cast<std::size_t>(chunks) * ChunkSize);
        const int violations = getNumberOfScratchViolations(WorkItems, chunks, scratch.getData());

        THEN("Every work item stays inside its chunk") { VALIDATE_EQUALS(violations, 0); }
      }
    }
  }
}

UNIT_TEST("The chunks cover the team of the enclosing parallel region even when a nested region would run one thread") {
  GIVEN("A dispatch of 1000 work items") {
    constexpr int WorkItems = 1000;

    WHEN("getNumberOfScratchChunks() returns the chunk count while a nested region would run one thread") {
      const int maximumThreads = omp_get_max_threads();
      omp_set_num_threads(1);
      const int chunks = tarch::accelerator::getNumberOfScratchChunks(WorkItems);
      omp_set_num_threads(maximumThreads);

      AND_WHEN("Every work item writes into its chunk of a device scratch buffer and reads the chunk back") {
        tarch::accelerator::Buffer<int> scratch{tarch::MemoryLocation::Device};
        scratch.allocate(static_cast<std::size_t>(chunks) * ChunkSize);
        const int violations = getNumberOfScratchViolations(WorkItems, chunks, scratch.getData());

        THEN("Every work item stays inside its chunk") { VALIDATE_EQUALS(violations, 0); }
      }
    }
  }
}

UNIT_TEST(
  "A dispatch runs at least one team, at most one team per work item and on a device at most its resident teams"
) {
  GIVEN("The threads of a default team and twice the number of teams that the device keeps resident at once") {
    const int teamThreads     = tarch::accelerator::getTeamThreads(tarch::accelerator::TeamRange::BackendDefault);
    const int concurrentTeams = tarch::accelerator::Device::getInstance().getNumberOfConcurrentThreads() / teamThreads;
    const int workItems       = 2 * std::max(1, concurrentTeams);

    WHEN("The persistent teams of these work items, of zero work items and of one work item are counted") {
      const int teams            = tarch::accelerator::getNumberOfPersistentTeams(workItems, teamThreads);
      const int teamsOfZeroItems = tarch::accelerator::getNumberOfPersistentTeams(0, teamThreads);
      const int teamsOfOneItem   = tarch::accelerator::getNumberOfPersistentTeams(1, teamThreads);

      THEN("The work items run on one team or more and on one team per work item or fewer") {
        VALIDATE(teams >= 1 and teams <= workItems, teams, workItems);
        VALIDATE_EQUALS(teamsOfZeroItems, 1);
        VALIDATE_EQUALS(teamsOfOneItem, 1);
#if defined(PEANO_WITH_GPU_BACKEND)
        VALIDATE(teams <= std::max(1, concurrentTeams), teams, concurrentTeams);
#else
        VALIDATE_EQUALS(teams, workItems);
#endif
      }
    }
  }
}
