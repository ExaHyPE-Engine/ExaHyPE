// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelConstructsTest.h"

#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tests/TestMacros.h"

namespace {
  TARGET_DECLARE
  constexpr int NumberOfEntries = 64;

  struct TwoMaxima final {
    double first{0.0};
    double second{0.0};
  };

  struct TwoMaximaOp final {
    using ValueType = TwoMaxima;

    static GPU_CALLABLE_METHOD ValueType identity() { return TwoMaxima{0.0, 0.0}; }

    static GPU_CALLABLE_METHOD ValueType combine(const ValueType left, const ValueType right) {
      return TwoMaxima{
        (left.first > right.first) ? left.first : right.first,
        (left.second > right.second) ? left.second : right.second
      };
    }
  };

  struct WriteDoubledIndex final {
    double* values{nullptr};
    int     extentY{1};
    int     extentZ{1};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index) const {
      values[index] = static_cast<double>(2 * index);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int x, const int y) const { (*this)(x* extentY + y); }

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int x, const int y, const int z) const {
      (*this)((x * extentY + y) * extentZ + z);
    }
  };

  struct WriteDoubledIndexPerTeam final {
    double* values{nullptr};
    int     teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      double* const entries = values;
      const int     base    = team * teamSize;
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(teamSize), [=](const int index) {
        entries[base + index] = static_cast<double>(2 * (base + index));
      });
    }
  };

  struct WriteDoubledIndexPerTeam2D final {
    double* values{nullptr};
    int     extentX{0};
    int     extentY{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      double* const entries = values;
      const int     stride  = extentY;
      const int     base    = team * extentX * extentY;
      tarch::accelerator::parallelFor(
        teamHandle,
        tarch::accelerator::RangeMD<2>(extentX, extentY),
        [=](const int x, const int y) {
          const int index = base + x * stride + y;
          entries[index]  = static_cast<double>(2 * index);
        }
      );
    }
  };

  struct WriteDoubledIndexPerTeam3D final {
    double* values{nullptr};
    int     extentX{0};
    int     extentY{0};
    int     extentZ{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      double* const entries = values;
      const int     strideX = extentY * extentZ;
      const int     strideY = extentZ;
      const int     base    = team * extentX * strideX;
      tarch::accelerator::parallelFor(
        teamHandle,
        tarch::accelerator::RangeMD<3>(extentX, extentY, extentZ),
        [=](const int x, const int y, const int z) {
          const int index = base + x * strideX + y * strideY + z;
          entries[index]  = static_cast<double>(2 * index);
        }
      );
    }
  };

  struct MirrorWithinTeam final {
    double* scratch{nullptr};
    double* values{nullptr};
    int     teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      double* const source = scratch;
      double* const target = values;
      const int     size   = teamSize;
      const int     base   = team * size;
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(size), [=](const int index) {
        source[base + index] = static_cast<double>(base + index);
      });
      tarch::accelerator::teamBarrier(teamHandle);
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(size), [=](const int index) {
        target[base + index] = 2.0 * source[base + size - 1 - index];
      });
    }
  };

  struct AddEntry final {
    const double* values{nullptr};
    int           extentY{1};
    int           extentZ{1};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, double& accumulator) const {
      accumulator += values[index];
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int x, const int y, double& accumulator) const {
      (*this)(x* extentY + y, accumulator);
    }

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int x,
      const int y,
      const int z,
      double&   accumulator
    ) const {
      (*this)((x * extentY + y) * extentZ + z, accumulator);
    }
  };

  struct RaiseMaximumPerTeam final {
    const double* values{nullptr};
    int           teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      double&                              teamAccumulator
    ) const {
      const double* const entries = values;
      const int           base    = team * teamSize;
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(teamSize),
        [=](const int index, double& accumulator) {
          const double value = entries[base + index];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    }
  };

  struct RaiseMaximumPerTeam2D final {
    const double* values{nullptr};
    int           extentX{0};
    int           extentY{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      double&                              teamAccumulator
    ) const {
      const double* const entries = values;
      const int           stride  = extentY;
      const int           base    = team * extentX * extentY;
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<2>(extentX, extentY),
        [=](const int x, const int y, double& accumulator) {
          const double value = entries[base + x * stride + y];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    }
  };

  struct RaiseMaximumPerTeam3D final {
    const double* values{nullptr};
    int           extentX{0};
    int           extentY{0};
    int           extentZ{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      double&                              teamAccumulator
    ) const {
      const double* const entries = values;
      const int           strideX = extentY * extentZ;
      const int           strideY = extentZ;
      const int           base    = team * extentX * strideX;
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<3>(extentX, extentY, extentZ),
        [=](const int x, const int y, const int z, double& accumulator) {
          const double value = entries[base + x * strideX + y * strideY + z];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    }
  };

  struct AddIndicesPerTeam final {
    int teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      double&                              teamAccumulator
    ) const {
      const int base = team * teamSize;
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(teamSize),
        [=](const int index, double& accumulator) { accumulator += base + index + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<2>(2, 4),
        [=](const int x, const int y, double& accumulator) { accumulator += base + x * 4 + y + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<3>(2, 2, 2),
        [=](const int x, const int y, const int z, double& accumulator) {
          accumulator += base + (x * 2 + y) * 2 + z + 1;
        },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(0),
        [](const int index, double& accumulator) { accumulator += index + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
    }
  };

  struct RaiseTwoMaximaPerTeam final {
    const double* values{nullptr};
    int           teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      TwoMaxima&                           teamAccumulator
    ) const {
      const double* const entries = values;
      const int           base    = team * teamSize;
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(teamSize),
        [=](const int index, TwoMaxima& accumulator) {
          const double value         = entries[base + index];
          const double mirroredValue = 2.0 * NumberOfEntries - value;
          accumulator.first          = (value > accumulator.first) ? value : accumulator.first;
          accumulator.second         = (mirroredValue > accumulator.second) ? mirroredValue : accumulator.second;
        },
        tarch::accelerator::Custom<TwoMaximaOp>{teamAccumulator}
      );
    }
  };

  struct RaiseTwoMaxima final {
    const double* values{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, TwoMaxima& accumulator) const {
      const double value         = values[index];
      const double mirroredValue = 2.0 * NumberOfEntries - value;
      accumulator.first          = (value > accumulator.first) ? value : accumulator.first;
      accumulator.second         = (mirroredValue > accumulator.second) ? mirroredValue : accumulator.second;
    }
  };

  /**
   * Copies the entries of one team into the team scratch and writes twice the mirrored entries
   * back. Every entry passes through the scratch of another thread.
   */
  struct MirrorThroughTeamScratch final {
    double* values{nullptr};
    int     teamSize{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      double* const entries = values + static_cast<std::ptrdiff_t>(team) * teamSize;
      auto* const   scratch = reinterpret_cast<double*>(tarch::accelerator::getTeamScratch(teamHandle));
      const int     size    = teamSize;
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(size), [=](const int index) {
        scratch[index] = entries[index];
      });
      tarch::accelerator::teamBarrier(teamHandle);
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(size), [=](const int index) {
        entries[index] = 2.0 * scratch[size - 1 - index];
      });
    }
  };

  /**
   * Adds the one-based index of every thread of one team to the accumulator of the team.
   */
  struct AddThreadIndicesPerTeam final {
    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      [[maybe_unused]] const int           team,
      const tarch::accelerator::TeamHandle teamHandle,
      double&                              teamAccumulator
    ) const {
      const auto threadValue = static_cast<double>(tarch::accelerator::getThreadOfTeam(teamHandle) + 1);
      tarch::accelerator::reduceOverTeam(teamHandle, threadValue, tarch::accelerator::Sum<double>{teamAccumulator});
    }
  };

  /**
   * Writes the result of two votes of a team for every thread of the team. Thread team % threads of
   * an even team votes yes in the vote of any thread. Thread team % threads of an odd team votes no
   * in the vote of every thread.
   */
  struct VoteInTeam final {
    int* anyThreadVotes{nullptr};
    int* everyThreadVotes{nullptr};
    int  entriesPerTeam{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle
    ) const {
      const int  thread            = tarch::accelerator::getThreadOfTeam(teamHandle);
      const bool isVoter           = thread == team % tarch::accelerator::getNumberOfThreadsPerTeam(teamHandle);
      const bool isEvenTeam        = team % 2 == 0;
      const bool isTrueInAnyThread = tarch::accelerator::isTrueInAnyThreadOfTeam(teamHandle, isEvenTeam and isVoter);
      const bool isTrueInEveryThread =
        tarch::accelerator::isTrueInEveryThreadOfTeam(teamHandle, isEvenTeam or not isVoter);
      anyThreadVotes[team * entriesPerTeam + thread]   = isTrueInAnyThread ? 1 : 0;
      everyThreadVotes[team * entriesPerTeam + thread] = isTrueInEveryThread ? 1 : 0;
    }
  };

  /**
   * Adds the result of a vote of any thread of the team for every thread of the team. Thread
   * team % threads of an even team votes yes.
   */
  struct CountVotesPerTeam final {
    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      int&                                 teamAccumulator
    ) const {
      const int  thread  = tarch::accelerator::getThreadOfTeam(teamHandle);
      const bool isVoter = thread == team % tarch::accelerator::getNumberOfThreadsPerTeam(teamHandle);
      const int  vote    = tarch::accelerator::isTrueInAnyThreadOfTeam(teamHandle, team % 2 == 0 and isVoter) ? 1 : 0;
      tarch::accelerator::reduceOverTeam(teamHandle, vote, tarch::accelerator::Sum<int>{teamAccumulator});
    }
  };

  /**
   * Lowers the accumulator of the team to the number of threads the smallest team runs.
   */
  struct LowerNumberOfThreadsPerTeam final {
    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      [[maybe_unused]] const int           team,
      const tarch::accelerator::TeamHandle teamHandle,
      int&                                 teamAccumulator
    ) const {
      tarch::accelerator::reduceOverTeam(
        teamHandle,
        tarch::accelerator::getNumberOfThreadsPerTeam(teamHandle),
        tarch::accelerator::Min<int>{teamAccumulator}
      );
    }
  };

  /**
   * Raises the accumulator of the team to the number of threads the largest team runs.
   */
  struct RaiseNumberOfThreadsPerTeam final {
    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      [[maybe_unused]] const int           team,
      const tarch::accelerator::TeamHandle teamHandle,
      int&                                 teamAccumulator
    ) const {
      tarch::accelerator::reduceOverTeam(
        teamHandle,
        tarch::accelerator::getNumberOfThreadsPerTeam(teamHandle),
        tarch::accelerator::Max<int>{teamAccumulator}
      );
    }
  };

  struct LowerMinimumEntry final {
    const double* values{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, double& accumulator) const {
      accumulator = (values[index] < accumulator) ? values[index] : accumulator;
    }
  };

  struct RaiseMaximumEntry final {
    const double* values{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, double& accumulator) const {
      accumulator = (values[index] > accumulator) ? values[index] : accumulator;
    }
  };
  TARGET_DECLARE_END
} // namespace

tarch::accelerator::tests::ParallelConstructsTest::ParallelConstructsTest() :
  TestCase("tarch::accelerator::tests::ParallelConstructsTest") {}

void
tarch::accelerator::tests::ParallelConstructsTest::run() {
  testMethod(testParallelForOnHost);
  testMethod(testParallelFor);
  testMethod(testParallelForRangeMD2);
  testMethod(testParallelForRangeMD3);
  testMethod(testParallelForTeam);
  testMethod(testParallelForTeamRangeMD2);
  testMethod(testParallelForTeamRangeMD3);
  testMethod(testTeamBarrier);
  testMethod(testEveryThreadOfATeamReceivesTheVotesOfTheTeam);
  testMethod(testParallelReduceSum);
  testMethod(testParallelReduceMin);
  testMethod(testParallelReduceMax);
  testMethod(testParallelReduceRangeMD2);
  testMethod(testParallelReduceRangeMD3);
  testMethod(testParallelReduceTeam);
  testMethod(testParallelReduceTeamSum);
  testMethod(testParallelReduceTeamRangeMD2);
  testMethod(testParallelReduceTeamRangeMD3);
  testMethod(testParallelReduceCustom);
  testMethod(testParallelReduceCustomRange);
  testMethod(testReductionScratchKeepsAnAddressThatFits);
  testMethod(testWaitForDeviceCompletesTheLaunchedLoop);
  testMethod(testAReductionOverManyTeamsSumsEveryTeam);
  testMethod(testAReductionOverTeamsOfThreeWarpsSumsEveryTeam);
  testMethod(testReduceOverTeamCombinesEveryThread);
  testMethod(testEveryTeamRunsTheDefaultNumberOfThreads);
  testMethod(testATeamLaunchRunsTheRequestedThreads);
  testMethod(testATeamLaunchStopsAtTheDefaultNumberOfThreads);
  testMethod(testTeamScratchMirrorsWithinTheTeam);
  testMethod(testTheDeviceRunsADefaultTeamAtOnce);
  testMethod(testTheNumberOfHostTasksFollowsTheTimePerItem);
#if not defined(PEANO_WITH_GPU_BACKEND)
  testMethod(testAHostReductionMergesItsBlocksInOrder);
#endif
}

/**
 * A host loop gets tasks of MinimumSecondsPerHostTask seconds between one task per thread and
 * MaximumHostTasksPerThread tasks per thread.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testTheNumberOfHostTasksFollowsTheTimePerItem() {
  constexpr int    Threads            = 4;
  constexpr int    Items              = 1000;
  constexpr int    TasksOfMinimumWork = 30;
  constexpr double SecondsPerItem     = TasksOfMinimumWork * tarch::accelerator::MinimumSecondsPerHostTask / Items;
  constexpr int    MostTasks          = tarch::accelerator::MaximumHostTasksPerThread * Threads;
  static_assert(Threads < TasksOfMinimumWork and TasksOfMinimumWork < MostTasks and MostTasks < Items);

  validateEquals(tarch::accelerator::getNumberOfHostTasks(Items, Threads, 0.0), Threads);
  validateEquals(tarch::accelerator::getNumberOfHostTasks(Items, Threads, SecondsPerItem), TasksOfMinimumWork);
  validateEquals(tarch::accelerator::getNumberOfHostTasks(Items, Threads, 1.0), MostTasks);
  validateEquals(tarch::accelerator::getNumberOfHostTasks(MostTasks / 2, Threads, 1.0), MostTasks / 2);
  validateEquals(tarch::accelerator::getNumberOfHostTasks(Threads / 2, Threads, 0.0), Threads / 2);
  validateEquals(tarch::accelerator::getNumberOfHostTasks(0, Threads, 0.0), 1);
}

#if not defined(PEANO_WITH_GPU_BACKEND)
/**
 * Every call of a host reduction merges the sums of its blocks in block order, whatever number of
 * tasks the time of the previous call sets.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testAHostReductionMergesItsBlocksInOrder() {
  constexpr int Entries         = 10000;
  constexpr int TermsPerEntry   = 50;
  const auto    addTermsOfEntry = [](const int entry, double& sum) {
    for (int term = 1; term <= TermsPerEntry; ++term) {
      sum += 1.0 / static_cast<double>(entry * TermsPerEntry + term);
    }
  };

  const int threads     = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int blocks      = std::min(Entries, tarch::accelerator::MaximumHostTasksPerThread * threads);
  double    sumOfBlocks = 0.0;
  for (int block = 0; block < blocks; ++block) {
    double sumOfBlock = 0.0;
    for (int entry = Entries * block / blocks; entry < Entries * (block + 1) / blocks; ++entry) {
      addTermsOfEntry(entry, sumOfBlock);
    }
    sumOfBlocks += sumOfBlock;
  }

  constexpr int Calls             = 3;
  int           callsWithBlockSum = 0;
  for (int call = 0; call < Calls; ++call) {
    double result = 0.0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(Entries),
      addTermsOfEntry,
      tarch::accelerator::Sum<double>{result}
    );
    callsWithBlockSum += (result == sumOfBlocks) ? 1 : 0;
  }
  validateEqualsWithParams1(callsWithBlockSum, Calls, sumOfBlocks);
}
#endif

void
tarch::accelerator::tests::ParallelConstructsTest::testTheDeviceRunsADefaultTeamAtOnce() {
  const int threads = tarch::accelerator::Device::getInstance().getNumberOfConcurrentThreads();
  validateWithParams1(threads >= tarch::accelerator::DefaultThreadsPerTeam, threads);
}

/**
 * Every team reads back through its scratch of TeamScratchBytes what its threads wrote there.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testTeamScratchMirrorsWithinTheTeam() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  static_assert(TeamSize * sizeof(double) <= tarch::accelerator::TeamScratchBytes);
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    MirrorThroughTeamScratch{data, TeamSize}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int team = 0; team < NumberOfTeams; ++team) {
    for (int i = 0; i < TeamSize; ++i) {
      const double expected = 2.0 * static_cast<double>(team * TeamSize + TeamSize - 1 - i + 1);
      correct               = correct and (hostValues[team * TeamSize + i] == expected);
    }
  }
  validate(correct);
}

/**
 * The team reduction of the one-based thread indices returns their sum over every thread of every
 * team.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testReduceOverTeamCombinesEveryThread() {
  constexpr int NumberOfTeams = 5;
  constexpr int Threads       = tarch::accelerator::DefaultThreadsPerTeam;
  double        result        = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    AddThreadIndicesPerTeam{},
    tarch::accelerator::Sum<double>{result}
  );
  validateWithParams1(result == NumberOfTeams * (Threads * (Threads + 1) / 2.0), result);
}

/**
 * Every team of a launch with TeamRange::BackendDefault runs at least DefaultThreadsPerTeam
 * threads.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testEveryTeamRunsTheDefaultNumberOfThreads() {
  constexpr int NumberOfTeams = 5;
  int           fewestThreads = std::numeric_limits<int>::max();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    LowerNumberOfThreadsPerTeam{},
    tarch::accelerator::Min<int>{fewestThreads}
  );
  validateWithParams1(fewestThreads >= tarch::accelerator::DefaultThreadsPerTeam, fewestThreads);
}

/**
 * Every team of a launch that asks for a number of threads and bytes of scratch runs
 * getTeamThreads() of that number and mirrors its values through the requested scratch.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testATeamLaunchRunsTheRequestedThreads() {
  constexpr int                       NumberOfTeams    = 6;
  constexpr int                       RequestedThreads = 40;
  constexpr int                       TeamSize         = 8;
  constexpr int                       Entries          = NumberOfTeams * TeamSize;
  constexpr int                       ScratchBytes     = TeamSize * static_cast<int>(sizeof(double));
  const int                           expectedThreads  = tarch::accelerator::getTeamThreads(RequestedThreads);
  const tarch::accelerator::TeamRange range(NumberOfTeams, RequestedThreads, ScratchBytes);

  int fewestThreads = std::numeric_limits<int>::max();
  int mostThreads   = 0;
  tarch::accelerator::parallelReduce(range, LowerNumberOfThreadsPerTeam{}, tarch::accelerator::Min<int>{fewestThreads});
  tarch::accelerator::parallelReduce(range, RaiseNumberOfThreadsPerTeam{}, tarch::accelerator::Max<int>{mostThreads});
  validateEqualsWithParams1(fewestThreads, expectedThreads, RequestedThreads);
  validateEqualsWithParams1(mostThreads, expectedThreads, RequestedThreads);

  auto* const data = tarch::allocateMemory<double>(Entries, tarch::MemoryLocation::Device);

  std::array<double, Entries> hostValues{};
  for (int i = 0; i < Entries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), Entries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(range, MirrorThroughTeamScratch{data, TeamSize});

  tarch::copyFrom(data, hostValues.data(), Entries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int team = 0; team < NumberOfTeams; ++team) {
    for (int i = 0; i < TeamSize; ++i) {
      const double expected = 2.0 * static_cast<double>(team * TeamSize + TeamSize - 1 - i + 1);
      correct               = correct and (hostValues[team * TeamSize + i] == expected);
    }
  }
  validate(correct);
}

/**
 * Every team of a launch that asks for more threads than DefaultThreadsPerTeam runs
 * DefaultThreadsPerTeam threads.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testATeamLaunchStopsAtTheDefaultNumberOfThreads() {
  constexpr int                       NumberOfTeams    = 6;
  constexpr int                       RequestedThreads = 4 * tarch::accelerator::DefaultThreadsPerTeam;
  const tarch::accelerator::TeamRange range(
    NumberOfTeams,
    RequestedThreads,
    tarch::accelerator::TeamRange::BackendDefault
  );

  int fewestThreads = std::numeric_limits<int>::max();
  int mostThreads   = 0;
  tarch::accelerator::parallelReduce(range, LowerNumberOfThreadsPerTeam{}, tarch::accelerator::Min<int>{fewestThreads});
  tarch::accelerator::parallelReduce(range, RaiseNumberOfThreadsPerTeam{}, tarch::accelerator::Max<int>{mostThreads});
  validateEqualsWithParams1(fewestThreads, tarch::accelerator::DefaultThreadsPerTeam, RequestedThreads);
  validateEqualsWithParams1(mostThreads, tarch::accelerator::DefaultThreadsPerTeam, RequestedThreads);
}

/**
 * A team reduction over more teams than the host combines directly returns the full sum.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testAReductionOverManyTeamsSumsEveryTeam() {
  constexpr int    NumberOfTeams = 20000;
  constexpr int    TeamSize      = 8;
  constexpr double InitialResult = 17.0;
  double           result        = InitialResult;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    AddIndicesPerTeam{TeamSize},
    tarch::accelerator::Sum<double>{result}
  );
  constexpr double Entries = NumberOfTeams * TeamSize;
  validateWithParams1(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
}

/**
 * A team reduction over teams of three warps returns the full sum.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testAReductionOverTeamsOfThreeWarpsSumsEveryTeam() {
  constexpr int    NumberOfTeams = 1000;
  constexpr int    TeamSize      = 8;
  constexpr double InitialResult = 17.0;
  double           result        = InitialResult;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams, 3 * tarch::accelerator::ThreadsPerWarp, 0),
    AddIndicesPerTeam{TeamSize},
    tarch::accelerator::Sum<double>{result}
  );
  constexpr double Entries = NumberOfTeams * TeamSize;
  validateWithParams1(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
}

/**
 * After waitForDevice() the host reads every value a loop wrote into pinned memory.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testWaitForDeviceCompletesTheLaunchedLoop() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Pinned);
  for (int i = 0; i < NumberOfEntries; ++i) {
    data[i] = -1.0;
  }

  tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfEntries), WriteDoubledIndex{data});
  tarch::accelerator::waitForDevice();

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (data[i] == static_cast<double>(2 * i));
  }
  tarch::freeMemory(data, tarch::MemoryLocation::Pinned);
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForOnHost() {
  std::vector<double> hostValues(NumberOfEntries, 0.0);
  tarch::accelerator::parallelForOnHost(tarch::accelerator::Range(NumberOfEntries), [&hostValues](const int i) {
    hostValues[i] = static_cast<double>(2 * i);
  });

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelFor() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfEntries), WriteDoubledIndex{data});

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD2() {
  constexpr int ExtentX = 8;
  constexpr int ExtentY = 8;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(tarch::accelerator::RangeMD<2>(ExtentX, ExtentY), WriteDoubledIndex{data, ExtentY});

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD3() {
  constexpr int ExtentX = 4;
  constexpr int ExtentY = 4;
  constexpr int ExtentZ = 4;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
    WriteDoubledIndex{data, ExtentY, ExtentZ}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeam() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    WriteDoubledIndexPerTeam{data, TeamSize}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeamRangeMD2() {
  constexpr int NumberOfTeams = 4;
  constexpr int ExtentX       = 4;
  constexpr int ExtentY       = 4;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    WriteDoubledIndexPerTeam2D{data, ExtentX, ExtentY}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeamRangeMD3() {
  constexpr int NumberOfTeams = 8;
  constexpr int ExtentX       = 2;
  constexpr int ExtentY       = 2;
  constexpr int ExtentZ       = 2;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    WriteDoubledIndexPerTeam3D{data, ExtentX, ExtentY, ExtentZ}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

/**
 * teamBarrier() makes the writes of every thread of a team visible to the other threads of the team.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testTeamBarrier() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   scratch       = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(scratch, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    MirrorWithinTeam{scratch, data, TeamSize}
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
  tarch::freeMemory(scratch, tarch::MemoryLocation::Device);
  tarch::freeMemory(data, tarch::MemoryLocation::Device);

  bool correct = true;
  for (int team = 0; team < NumberOfTeams; ++team) {
    for (int i = 0; i < TeamSize; ++i) {
      const double expected = 2.0 * static_cast<double>(team * TeamSize + TeamSize - 1 - i);
      correct               = correct and (hostValues[team * TeamSize + i] == expected);
    }
  }
  validate(correct);
}

/**
 * Every thread of a team of two warps receives the result of the votes of all threads of its team.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testEveryThreadOfATeamReceivesTheVotesOfTheTeam() {
  constexpr int NumberOfTeams    = 6;
  const int     threads          = tarch::accelerator::getTeamThreads(2 * tarch::accelerator::ThreadsPerWarp);
  const int     numberOfVotes    = NumberOfTeams * threads;
  auto* const   anyThreadVotes   = tarch::allocateMemory<int>(numberOfVotes, tarch::MemoryLocation::Device);
  auto* const   everyThreadVotes = tarch::allocateMemory<int>(numberOfVotes, tarch::MemoryLocation::Device);

  const tarch::accelerator::TeamRange range(NumberOfTeams, 2 * tarch::accelerator::ThreadsPerWarp, 0);
  tarch::accelerator::parallelFor(range, VoteInTeam{anyThreadVotes, everyThreadVotes, threads});
  int votesOfEvenTeams = 0;
  tarch::accelerator::parallelReduce(range, CountVotesPerTeam{}, tarch::accelerator::Sum<int>{votesOfEvenTeams});

  std::vector<int> hostAnyThreadVotes(numberOfVotes, -1);
  std::vector<int> hostEveryThreadVotes(numberOfVotes, -1);
  tarch::copyFrom(anyThreadVotes, hostAnyThreadVotes.data(), numberOfVotes, tarch::MemoryLocation::Device);
  tarch::copyFrom(everyThreadVotes, hostEveryThreadVotes.data(), numberOfVotes, tarch::MemoryLocation::Device);
  tarch::freeMemory(anyThreadVotes, tarch::MemoryLocation::Device);
  tarch::freeMemory(everyThreadVotes, tarch::MemoryLocation::Device);

  for (int vote = 0; vote < numberOfVotes; vote++) {
    const int expected = (vote / threads) % 2 == 0 ? 1 : 0;
    validateEqualsWithParams2(hostAnyThreadVotes[vote], expected, vote, threads);
    validateEqualsWithParams2(hostEveryThreadVotes[vote], expected, vote, threads);
  }
  validateEqualsWithParams1(votesOfEvenTeams, (NumberOfTeams + 1) / 2 * threads, threads);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceSum() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    AddEntry{data},
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD2() {
  constexpr int ExtentX = 8;
  constexpr int ExtentY = 8;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
    AddEntry{data, ExtentY},
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD3() {
  constexpr int ExtentX = 4;
  constexpr int ExtentY = 4;
  constexpr int ExtentZ = 4;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
    AddEntry{data, ExtentY, ExtentZ},
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeam() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    RaiseMaximumPerTeam{data, TeamSize},
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

/**
 * A team reduction adds the indices of every team to the value that seeds the sum.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamSum() {
  constexpr int    NumberOfTeams = 8;
  constexpr int    TeamSize      = 8;
  constexpr double InitialResult = 17.0;
  double           result        = InitialResult;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    AddIndicesPerTeam{TeamSize},
    tarch::accelerator::Sum<double>{result}
  );
  constexpr int Entries = NumberOfTeams * TeamSize;
  validateWithParams1(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamRangeMD2() {
  constexpr int NumberOfTeams = 4;
  constexpr int ExtentX       = 4;
  constexpr int ExtentY       = 4;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    RaiseMaximumPerTeam2D{data, ExtentX, ExtentY},
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamRangeMD3() {
  constexpr int NumberOfTeams = 8;
  constexpr int ExtentX       = 2;
  constexpr int ExtentY       = 2;
  constexpr int ExtentZ       = 2;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    RaiseMaximumPerTeam3D{data, ExtentX, ExtentY, ExtentZ},
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceCustom() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  TwoMaxima result{0.0, 0.0};
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    RaiseTwoMaximaPerTeam{data, TeamSize},
    tarch::accelerator::Custom<TwoMaximaOp>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams2(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
  validateWithParams2(result.second == static_cast<double>(2 * NumberOfEntries - 1), result.first, result.second);
}

/**
 * A custom reduction over a flat range keeps a seeded maximum that no entry exceeds.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceCustomRange() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  constexpr double SeededSecond = 3.0 * NumberOfEntries;
  TwoMaxima        result{0.0, SeededSecond};
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    RaiseTwoMaxima{data},
    tarch::accelerator::Custom<TwoMaximaOp>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams2(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
  validateWithParams2(result.second == SeededSecond, result.first, result.second);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMin() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = std::numeric_limits<double>::max();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    LowerMinimumEntry{data},
    tarch::accelerator::Min<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == 1.0, result);
}

void
tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMax() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    RaiseMaximumEntry{data},
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

/**
 * A reduction scratch request that fits the last allocation returns its address again.
 */
void
tarch::accelerator::tests::ParallelConstructsTest::testReductionScratchKeepsAnAddressThatFits() {
  void* const small = tarch::accelerator::lockReductionScratch(64);
  validate(small != nullptr);
  tarch::accelerator::unlockReductionScratch();
  void* const smallAgain = tarch::accelerator::lockReductionScratch(32);
  tarch::accelerator::unlockReductionScratch();
  validateEquals(smallAgain, small);
  void* const large = tarch::accelerator::lockReductionScratch(4096);
  validate(large != nullptr);
  tarch::accelerator::unlockReductionScratch();
  void* const largeAgain   = tarch::accelerator::lockReductionScratch(4096);
  void* const staging      = tarch::accelerator::getReductionStaging(16);
  void* const stagingAgain = tarch::accelerator::getReductionStaging(8);
  tarch::accelerator::unlockReductionScratch();
  validateEquals(largeAgain, large);
  validateEquals(stagingAgain, staging);
  tarch::accelerator::releaseReductionScratch();
  validate(tarch::accelerator::lockReductionScratch(8) != nullptr);
  tarch::accelerator::unlockReductionScratch();
}
