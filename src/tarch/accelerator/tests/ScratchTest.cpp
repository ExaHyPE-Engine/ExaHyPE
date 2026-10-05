// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ScratchTest.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/Scratch.h"
#include "tarch/accelerator/accelerator.h"

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

tarch::accelerator::tests::ScratchTest::ScratchTest() :
  tarch::tests::TestCase("tarch::accelerator::tests::ScratchTest") {}

void
tarch::accelerator::tests::ScratchTest::run() {
  testMethod(testEveryWorkItemNamesAChunkOfTheDispatch);
  testMethod(testTheChunksCoverTheTeamOfTheEnclosingRegion);
}

/**
 * Every work item of a dispatch gets a chunk below the chunk count and reads back what it writes
 * into it.
 */
void
tarch::accelerator::tests::ScratchTest::testEveryWorkItemNamesAChunkOfTheDispatch() {
  constexpr int WorkItems    = 1000;
  const int     chunks       = tarch::accelerator::getNumberOfScratchChunks(WorkItems);
  const int     chunksOfNone = tarch::accelerator::getNumberOfScratchChunks(0);
  validate(chunks >= 1);
  validate(chunksOfNone == 0 or chunksOfNone == chunks);

  tarch::accelerator::Buffer<int> scratch{tarch::MemoryLocation::Device};
  scratch.allocate(static_cast<std::size_t>(chunks) * ChunkSize);
  validateEquals(getNumberOfScratchViolations(WorkItems, chunks, scratch.getData()), 0);
  scratch.release();
}

/**
 * The chunks cover the team of the enclosing parallel region even when a nested region would run one
 * thread.
 */
void
tarch::accelerator::tests::ScratchTest::testTheChunksCoverTheTeamOfTheEnclosingRegion() {
  constexpr int WorkItems      = 1000;
  const int     maximumThreads = omp_get_max_threads();
  omp_set_num_threads(1);
  const int chunks = tarch::accelerator::getNumberOfScratchChunks(WorkItems);
  omp_set_num_threads(maximumThreads);

  tarch::accelerator::Buffer<int> scratch{tarch::MemoryLocation::Device};
  scratch.allocate(static_cast<std::size_t>(chunks) * ChunkSize);
  validateEquals(getNumberOfScratchViolations(WorkItems, chunks, scratch.getData()), 0);
}
