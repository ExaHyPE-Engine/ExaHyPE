// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/testing/UnitTest.h"

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
   * an even team votes true in the vote of any thread. Thread team % threads of an odd team votes
   * false in the vote of every thread.
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

  bool everyEntryStoresTwiceItsIndex(const std::span<const double> values) {
    bool correct = true;
    for (std::size_t index = 0; index < values.size(); index++) {
      correct = correct and (values[index] == static_cast<double>(2 * index));
    }
    return correct;
  }

  /**
   * Returns whether every entry equals twice the initial value of its mirror entry within its team.
   */
  bool everyTeamStoresTwiceTheMirroredSequence(
    const std::span<const double> values,
    const int                     numberOfTeams,
    const int                     teamSize
  ) {
    bool correct = true;
    for (int team = 0; team < numberOfTeams; team++) {
      for (int index = 0; index < teamSize; index++) {
        const double expected = 2.0 * static_cast<double>(team * teamSize + teamSize - index);
        correct               = correct and (values[static_cast<std::size_t>(team * teamSize + index)] == expected);
      }
    }
    return correct;
  }

  /**
   * Returns the entries of a device array after launch wrote into it. Every entry starts as its index
   * plus one.
   */
  template <typename Launch>
  std::vector<double> getEntriesAfterTheLaunch(const int numberOfEntries, Launch&& launch) {
    double* const       data = tarch::allocateMemory<double>(numberOfEntries, tarch::MemoryLocation::Device);
    std::vector<double> hostValues(static_cast<std::size_t>(numberOfEntries));
    for (int index = 0; index < numberOfEntries; index++) {
      hostValues[static_cast<std::size_t>(index)] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), numberOfEntries, tarch::MemoryLocation::Device);
    launch(data);
    tarch::copyFrom(data, hostValues.data(), numberOfEntries, tarch::MemoryLocation::Device);
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
    return hostValues;
  }
} // namespace

UNIT_TEST("A parallel loop on the host writes every entry of a host vector") {
  GIVEN("A host vector of 64 zeros") {
    std::vector<double> hostValues(NumberOfEntries, 0.0);

    WHEN("A parallel loop on the host writes twice the index into every entry") {
      tarch::accelerator::parallelForOnHost(tarch::accelerator::Range(NumberOfEntries), [&hostValues](const int index) {
        hostValues[index] = static_cast<double>(2 * index);
      });

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
  }
}

UNIT_TEST("A parallel loop over a range writes every entry of a device array") {
  GIVEN("A device array of 64 zeros") {
    auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel loop over the 64 entries writes twice the index into every entry") {
      tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfEntries), WriteDoubledIndex{data});
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel loop over a two-dimensional range writes every entry of a device array") {
  GIVEN("A device array of 64 zeros and the extents eight by eight") {
    constexpr int ExtentX = 8;
    constexpr int ExtentY = 8;
    auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel loop over the two-dimensional range writes twice the linear index into every entry") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
        WriteDoubledIndex{data, ExtentY}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel loop over a three-dimensional range writes every entry of a device array") {
  GIVEN("A device array of 64 zeros and the extents four by four by four") {
    constexpr int ExtentX = 4;
    constexpr int ExtentY = 4;
    constexpr int ExtentZ = 4;
    auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel loop over the three-dimensional range writes twice the linear index into every entry") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
        WriteDoubledIndex{data, ExtentY, ExtentZ}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel loop over teams writes every entry of a device array in a flat range per team") {
  GIVEN("A device array of 64 zeros and eight teams of eight entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int TeamSize      = 8;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team writes twice the linear index into every entry of its range") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::TeamRange(NumberOfTeams),
        WriteDoubledIndexPerTeam{data, TeamSize}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel loop over teams writes every entry of a device array in a two-dimensional range per team") {
  GIVEN("A device array of 64 zeros and four teams of four by four entries") {
    constexpr int NumberOfTeams = 4;
    constexpr int ExtentX       = 4;
    constexpr int ExtentY       = 4;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team writes twice the linear index into every entry of its two-dimensional range") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::TeamRange(NumberOfTeams),
        WriteDoubledIndexPerTeam2D{data, ExtentX, ExtentY}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel loop over teams writes every entry of a device array in a three-dimensional range per team") {
  GIVEN("A device array of 64 zeros and eight teams of two by two by two entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int ExtentX       = 2;
    constexpr int ExtentY       = 2;
    constexpr int ExtentZ       = 2;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team writes twice the linear index into every entry of its three-dimensional range") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::TeamRange(NumberOfTeams),
        WriteDoubledIndexPerTeam3D{data, ExtentX, ExtentY, ExtentZ}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

      THEN("Every entry equals twice its index") { VALIDATE(everyEntryStoresTwiceItsIndex(hostValues)); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A team barrier makes the writes of every team member visible to the other members of the team") {
  GIVEN("A device scratch array and a device array of 64 zeros each and eight teams of eight entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int TeamSize      = 8;
    auto* const   scratch       = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    tarch::copyTo(scratch, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team writes its indices into the scratch array and, after a barrier, reads them back mirrored") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::TeamRange(NumberOfTeams),
        MirrorWithinTeam{scratch, data, TeamSize}
      );
      tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);
      bool mirroredCorrectly = true;
      for (int team = 0; team < NumberOfTeams; ++team) {
        for (int index = 0; index < TeamSize; ++index) {
          const double expected = 2.0 * static_cast<double>(team * TeamSize + TeamSize - 1 - index);
          mirroredCorrectly     = mirroredCorrectly and (hostValues[team * TeamSize + index] == expected);
        }
      }

      THEN("Every entry equals twice the index at its mirrored position within the team") {
        VALIDATE(mirroredCorrectly);
      }
    }
    tarch::freeMemory(scratch, tarch::MemoryLocation::Device);
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("Every thread of a team of two warps receives the result of the votes of all threads of its team") {
  GIVEN("Six teams of two warps and one device array of votes per vote for every thread of every team") {
    constexpr int NumberOfTeams    = 6;
    const int     threads          = tarch::accelerator::getTeamThreads(2 * tarch::accelerator::ThreadsPerWarp);
    const int     numberOfVotes    = NumberOfTeams * threads;
    auto* const   anyThreadVotes   = tarch::allocateMemory<int>(numberOfVotes, tarch::MemoryLocation::Device);
    auto* const   everyThreadVotes = tarch::allocateMemory<int>(numberOfVotes, tarch::MemoryLocation::Device);
    const tarch::accelerator::TeamRange range(NumberOfTeams, 2 * tarch::accelerator::ThreadsPerWarp, 0);

    WHEN("One thread per team votes, every thread writes both votes and a reduction adds the votes of any thread") {
      tarch::accelerator::parallelFor(range, VoteInTeam{anyThreadVotes, everyThreadVotes, threads});
      int votesOfEvenTeams = 0;
      tarch::accelerator::parallelReduce(range, CountVotesPerTeam{}, tarch::accelerator::Sum<int>{votesOfEvenTeams});
      std::vector<int> hostAnyThreadVotes(numberOfVotes, -1);
      std::vector<int> hostEveryThreadVotes(numberOfVotes, -1);
      tarch::copyFrom(anyThreadVotes, hostAnyThreadVotes.data(), numberOfVotes, tarch::MemoryLocation::Device);
      tarch::copyFrom(everyThreadVotes, hostEveryThreadVotes.data(), numberOfVotes, tarch::MemoryLocation::Device);

      THEN("Every thread of an even team receives one, of an odd team zero and the sum counts the even teams") {
        for (int vote = 0; vote < numberOfVotes; vote++) {
          const int expected = (vote / threads) % 2 == 0 ? 1 : 0;
          VALIDATE_EQUALS(hostAnyThreadVotes[vote], expected, vote, threads);
          VALIDATE_EQUALS(hostEveryThreadVotes[vote], expected, vote, threads);
        }
        VALIDATE_EQUALS(votesOfEvenTeams, (NumberOfTeams + 1) / 2 * threads, threads);
      }
    }
    tarch::freeMemory(anyThreadVotes, tarch::MemoryLocation::Device);
    tarch::freeMemory(everyThreadVotes, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel sum over a range adds every entry of a device array") {
  GIVEN("A device array of 64 ones") {
    auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = 1.0;
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction sums the entries") {
      double result = 0.0;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(NumberOfEntries),
        AddEntry{data},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The sum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel minimum over a range finds the smallest entry of a device array") {
  GIVEN("A device array of the values one to 64") {
    auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction takes the minimum of the entries") {
      double result = std::numeric_limits<double>::max();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(NumberOfEntries),
        LowerMinimumEntry{data},
        tarch::accelerator::Min<double>{result}
      );

      THEN("The minimum is one") { VALIDATE(result == 1.0, result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel maximum over a range finds the largest entry of a device array") {
  GIVEN("A device array of the values one to 64") {
    auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction takes the maximum of the entries") {
      double result = std::numeric_limits<double>::lowest();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(NumberOfEntries),
        RaiseMaximumEntry{data},
        tarch::accelerator::Max<double>{result}
      );

      THEN("The maximum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel sum over a two-dimensional range adds every entry of a device array") {
  GIVEN("A device array of 64 ones and the extents eight by eight") {
    constexpr int ExtentX = 8;
    constexpr int ExtentY = 8;
    auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = 1.0;
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction over the two-dimensional range sums the entries") {
      double result = 0.0;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
        AddEntry{data, ExtentY},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The sum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel sum over a three-dimensional range adds every entry of a device array") {
  GIVEN("A device array of 64 ones and the extents four by four by four") {
    constexpr int ExtentX = 4;
    constexpr int ExtentY = 4;
    constexpr int ExtentZ = 4;
    auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = 1.0;
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("A parallel reduction over the three-dimensional range sums the entries") {
      double result = 0.0;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
        AddEntry{data, ExtentY, ExtentZ},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The sum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel maximum over teams finds the largest entry in a flat range per team") {
  GIVEN("A device array of the values one to 64 and eight teams of eight entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int TeamSize      = 8;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team reduces the entries of its range to their maximum") {
      double result = std::numeric_limits<double>::lowest();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        RaiseMaximumPerTeam{data, TeamSize},
        tarch::accelerator::Max<double>{result}
      );

      THEN("The maximum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("Consecutive team reductions add their sums to the seed of the result") {
  GIVEN("Eight teams of eight entries and the seed 17") {
    constexpr int    NumberOfTeams = 8;
    constexpr int    TeamSize      = 8;
    constexpr int    Entries       = NumberOfTeams * TeamSize;
    constexpr double InitialResult = 17.0;

    WHEN("Every team adds the one-based indices of its flat, its two- and its three-dimensional range to the result") {
      double result = InitialResult;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        AddIndicesPerTeam{TeamSize},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The result is the seed plus three times the sum of the one-based indices of all entries") {
        VALIDATE(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
      }
    }
  }
}

UNIT_TEST("A parallel maximum over teams finds the largest entry in a two-dimensional range per team") {
  GIVEN("A device array of the values one to 64 and four teams of four by four entries") {
    constexpr int NumberOfTeams = 4;
    constexpr int ExtentX       = 4;
    constexpr int ExtentY       = 4;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team reduces the entries of its two-dimensional range to their maximum") {
      double result = std::numeric_limits<double>::lowest();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        RaiseMaximumPerTeam2D{data, ExtentX, ExtentY},
        tarch::accelerator::Max<double>{result}
      );

      THEN("The maximum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A parallel maximum over teams finds the largest entry in a three-dimensional range per team") {
  GIVEN("A device array of the values one to 64 and eight teams of two by two by two entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int ExtentX       = 2;
    constexpr int ExtentY       = 2;
    constexpr int ExtentZ       = 2;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team reduces the entries of its three-dimensional range to their maximum") {
      double result = std::numeric_limits<double>::lowest();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        RaiseMaximumPerTeam3D{data, ExtentX, ExtentY, ExtentZ},
        tarch::accelerator::Max<double>{result}
      );

      THEN("The maximum is 64") { VALIDATE(result == static_cast<double>(NumberOfEntries), result); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A custom reduction over teams finds the largest entry of a device array and its largest mirror") {
  GIVEN("A device array of the values one to 64 and eight teams of eight entries") {
    constexpr int NumberOfTeams = 8;
    constexpr int TeamSize      = 8;
    auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("Every team raises the two maxima to the largest entry of its range and to its largest mirror") {
      TwoMaxima result{0.0, 0.0};
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        RaiseTwoMaximaPerTeam{data, TeamSize},
        tarch::accelerator::Custom<TwoMaximaOp>{result}
      );

      THEN("The first maximum is 64 and the second is the mirror of the entry one") {
        VALIDATE(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
        VALIDATE(result.second == static_cast<double>(2 * NumberOfEntries - 1), result.first, result.second);
      }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A custom reduction over a flat range keeps a seeded maximum above every entry") {
  GIVEN("A device array of the values one to 64 and the seed three times 64") {
    constexpr double SeededSecond = 3.0 * NumberOfEntries;
    auto* const      data         = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device);

    std::array<double, NumberOfEntries> hostValues{};
    for (int index = 0; index < NumberOfEntries; ++index) {
      hostValues[index] = static_cast<double>(index + 1);
    }
    tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device);

    WHEN("The two maxima, the second one from the seed, rise to the largest entry and its largest mirror") {
      TwoMaxima result{0.0, SeededSecond};
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(NumberOfEntries),
        RaiseTwoMaxima{data},
        tarch::accelerator::Custom<TwoMaximaOp>{result}
      );

      THEN("The first maximum is 64 and the second keeps its seed") {
        VALIDATE(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
        VALIDATE(result.second == SeededSecond, result.first, result.second);
      }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Device);
  }
}

UNIT_TEST("A team reduction over more teams than the host combines at once returns the sum over every team") {
  GIVEN("20000 teams of eight entries and the seed 17") {
    constexpr int    NumberOfTeams = 20000;
    constexpr int    TeamSize      = 8;
    constexpr double Entries       = NumberOfTeams * TeamSize;
    constexpr double InitialResult = 17.0;

    WHEN("Every team adds the one-based indices of its flat, its two- and its three-dimensional range to the result") {
      double result = InitialResult;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        AddIndicesPerTeam{TeamSize},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The result is the seed plus three times the sum of the one-based indices of all entries") {
        VALIDATE(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
      }
    }
  }
}

UNIT_TEST("A team reduction over teams of three warps returns the sum over every team") {
  GIVEN("1000 teams of three warps with eight entries each and the seed 17") {
    constexpr int    NumberOfTeams = 1000;
    constexpr int    TeamSize      = 8;
    constexpr double Entries       = NumberOfTeams * TeamSize;
    constexpr double InitialResult = 17.0;

    WHEN("Every team adds the one-based indices of its flat, its two- and its three-dimensional range to the result") {
      double result = InitialResult;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams, 3 * tarch::accelerator::ThreadsPerWarp, 0),
        AddIndicesPerTeam{TeamSize},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The result is the seed plus three times the sum of the one-based indices of all entries") {
        VALIDATE(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
      }
    }
  }
}

UNIT_TEST("A reduction over every team returns the sum of the one-based thread indices of every team") {
  GIVEN("Five teams of the default team size") {
    constexpr int NumberOfTeams = 5;
    constexpr int Threads       = tarch::accelerator::DefaultThreadsPerTeam;

    WHEN("Every thread adds its one-based index through a reduction over its team") {
      double result = 0.0;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        AddThreadIndicesPerTeam{},
        tarch::accelerator::Sum<double>{result}
      );

      THEN("The result is five times the sum of the one-based thread indices of one team") {
        VALIDATE(result == NumberOfTeams * (Threads * (Threads + 1) / 2.0), result);
      }
    }
  }
}

UNIT_TEST("Every team of a launch with TeamRange::BackendDefault runs at least DefaultThreadsPerTeam threads") {
  GIVEN("Five teams of the default team size") {
    constexpr int NumberOfTeams = 5;

    WHEN("A reduction lowers its result to the number of threads of every team") {
      int fewestThreads = std::numeric_limits<int>::max();
      tarch::accelerator::parallelReduce(
        tarch::accelerator::TeamRange(NumberOfTeams),
        LowerNumberOfThreadsPerTeam{},
        tarch::accelerator::Min<int>{fewestThreads}
      );

      THEN("The smallest team runs at least DefaultThreadsPerTeam threads") {
        VALIDATE(fewestThreads >= tarch::accelerator::DefaultThreadsPerTeam, fewestThreads);
      }
    }
  }
}

UNIT_TEST("Every team of a launch that asks for more threads than DefaultThreadsPerTeam runs DefaultThreadsPerTeam") {
  GIVEN("Six teams that ask for four times DefaultThreadsPerTeam threads") {
    constexpr int                       NumberOfTeams    = 6;
    constexpr int                       RequestedThreads = 4 * tarch::accelerator::DefaultThreadsPerTeam;
    const tarch::accelerator::TeamRange range(
      NumberOfTeams,
      RequestedThreads,
      tarch::accelerator::TeamRange::BackendDefault
    );

    WHEN("Two reductions lower and raise their results to the number of threads of every team") {
      int fewestThreads = std::numeric_limits<int>::max();
      int mostThreads   = 0;
      tarch::accelerator::parallelReduce(
        range,
        LowerNumberOfThreadsPerTeam{},
        tarch::accelerator::Min<int>{fewestThreads}
      );
      tarch::accelerator::parallelReduce(
        range,
        RaiseNumberOfThreadsPerTeam{},
        tarch::accelerator::Max<int>{mostThreads}
      );

      THEN("The smallest and the largest team run DefaultThreadsPerTeam threads") {
        VALIDATE_EQUALS(fewestThreads, tarch::accelerator::DefaultThreadsPerTeam, RequestedThreads);
        VALIDATE_EQUALS(mostThreads, tarch::accelerator::DefaultThreadsPerTeam, RequestedThreads);
      }
    }
  }
}

UNIT_TEST("A team launch with requested threads and scratch runs getTeamThreads() threads and mirrors through it") {
  GIVEN("A launch of six teams that requests 40 threads and the scratch of eight entries per team") {
    constexpr int                       NumberOfTeams    = 6;
    constexpr int                       RequestedThreads = 40;
    constexpr int                       TeamSize         = 8;
    constexpr int                       ScratchBytes     = TeamSize * static_cast<int>(sizeof(double));
    const int                           expectedThreads  = tarch::accelerator::getTeamThreads(RequestedThreads);
    const tarch::accelerator::TeamRange range(NumberOfTeams, RequestedThreads, ScratchBytes);

    WHEN("Two reductions find the number of threads of the smallest and of the largest team") {
      int fewestThreads = std::numeric_limits<int>::max();
      int mostThreads   = 0;
      tarch::accelerator::parallelReduce(
        range,
        LowerNumberOfThreadsPerTeam{},
        tarch::accelerator::Min<int>{fewestThreads}
      );
      tarch::accelerator::parallelReduce(
        range,
        RaiseNumberOfThreadsPerTeam{},
        tarch::accelerator::Max<int>{mostThreads}
      );

      THEN("Every team runs getTeamThreads() of the requested threads") {
        VALIDATE_EQUALS(fewestThreads, expectedThreads, RequestedThreads);
        VALIDATE_EQUALS(mostThreads, expectedThreads, RequestedThreads);
      }
    }

    WHEN("Every team of the launch mirrors its entries of a device array through its scratch") {
      const auto mirror = [&range](double* const data) {
        tarch::accelerator::parallelFor(range, MirrorThroughTeamScratch{data, TeamSize});
      };
      const std::vector<double> values = getEntriesAfterTheLaunch(NumberOfTeams * TeamSize, mirror);

      THEN("Every entry equals twice the initial value of its mirror entry within its team") {
        VALIDATE(everyTeamStoresTwiceTheMirroredSequence(values, NumberOfTeams, TeamSize));
      }
    }
  }
}

UNIT_TEST("Every team reads back through its scratch of TeamScratchBytes what its threads wrote there") {
  GIVEN("Eight teams of eight entries of a device array") {
    constexpr int NumberOfTeams = 8;
    constexpr int TeamSize      = 8;
    static_assert(TeamSize * sizeof(double) <= tarch::accelerator::TeamScratchBytes);

    WHEN("Every team copies its entries into its scratch and writes twice the mirrored entries back") {
      const std::vector<double> values = getEntriesAfterTheLaunch(NumberOfTeams * TeamSize, [](double* const data) {
        tarch::accelerator::parallelFor(
          tarch::accelerator::TeamRange(NumberOfTeams),
          MirrorThroughTeamScratch{data, TeamSize}
        );
      });

      THEN("Every entry equals twice the initial value of its mirror entry within its team") {
        VALIDATE(everyTeamStoresTwiceTheMirroredSequence(values, NumberOfTeams, TeamSize));
      }
    }
  }
}

UNIT_TEST("The device runs at least the threads of one default team at once") {
  GIVEN("The number of threads that the device runs at once") {
    const int threads = tarch::accelerator::Device::getInstance().getNumberOfConcurrentThreads();

    THEN("The device runs at least DefaultThreadsPerTeam threads at once") {
      VALIDATE(threads >= tarch::accelerator::DefaultThreadsPerTeam, threads);
    }
  }
}

UNIT_TEST("The number of host tasks follows the time per item within the bounds per thread and per item") {
  GIVEN("Four threads, 1000 items and the time per item that fills 30 tasks of MinimumSecondsPerHostTask") {
    constexpr int    Threads            = 4;
    constexpr int    Items              = 1000;
    constexpr int    TasksOfMinimumWork = 30;
    constexpr double SecondsPerItem     = TasksOfMinimumWork * tarch::accelerator::MinimumSecondsPerHostTask / Items;
    constexpr int    MostTasks          = tarch::accelerator::MaximumHostTasksPerThread * Threads;
    static_assert(Threads < TasksOfMinimumWork and TasksOfMinimumWork < MostTasks and MostTasks < Items);

    WHEN("The host computes the task counts for zero, that and one second per item and for fewer items") {
      const int tasksOfInstantItems = tarch::accelerator::getNumberOfHostTasks(Items, Threads, 0.0);
      const int tasksOfTimedItems   = tarch::accelerator::getNumberOfHostTasks(Items, Threads, SecondsPerItem);
      const int tasksOfSlowItems    = tarch::accelerator::getNumberOfHostTasks(Items, Threads, 1.0);
      const int tasksOfFewSlowItems = tarch::accelerator::getNumberOfHostTasks(MostTasks / 2, Threads, 1.0);
      const int tasksOfFewItems     = tarch::accelerator::getNumberOfHostTasks(Threads / 2, Threads, 0.0);
      const int tasksOfNoItem       = tarch::accelerator::getNumberOfHostTasks(0, Threads, 0.0);

      THEN("Instant items receive one task per thread and timed items tasks of MinimumSecondsPerHostTask") {
        VALIDATE_EQUALS(tasksOfInstantItems, Threads);
        VALIDATE_EQUALS(tasksOfTimedItems, TasksOfMinimumWork);
      }

      THEN("Slow items receive MaximumHostTasksPerThread tasks per thread and every loop at most one task per item") {
        VALIDATE_EQUALS(tasksOfSlowItems, MostTasks);
        VALIDATE_EQUALS(tasksOfFewSlowItems, MostTasks / 2);
        VALIDATE_EQUALS(tasksOfFewItems, Threads / 2);
      }

      THEN("A loop without items gets one task") { VALIDATE_EQUALS(tasksOfNoItem, 1); }
    }
  }
}

#if not defined(PEANO_WITH_GPU_BACKEND)
UNIT_TEST("Every call of a host reduction merges the sums of its blocks in block order") {
  GIVEN("Entries of 50 reciprocal terms each and the sum over the blocks of the most host tasks in block order") {
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

    WHEN("A host reduction sums the entries three times and every call sets the task count of the next one") {
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

      THEN("Every call returns the bits of the sum over the blocks") {
        VALIDATE_EQUALS(callsWithBlockSum, Calls, sumOfBlocks);
      }
    }
  }
}
#endif

UNIT_TEST("After waitForDevice() the host reads every value that a loop wrote into pinned memory") {
  GIVEN("A pinned array of 64 entries of minus one") {
    auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Pinned);
    for (int index = 0; index < NumberOfEntries; index++) {
      data[index] = -1.0;
    }

    WHEN("A parallel loop writes twice the index into every entry and the host waits for the device") {
      tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfEntries), WriteDoubledIndex{data});
      tarch::accelerator::waitForDevice();
      const bool correct = everyEntryStoresTwiceItsIndex(std::span<const double>(data, NumberOfEntries));

      THEN("Every entry equals twice its index") { VALIDATE(correct); }
    }
    tarch::freeMemory(data, tarch::MemoryLocation::Pinned);
  }
}

UNIT_TEST("A reduction scratch request that fits the last allocation returns its address again") {
  GIVEN("The reduction scratch of this process") {
    WHEN("Host code locks 64 and then 32 bytes, 4096 bytes twice and staging of 16 and then eight bytes") {
      void* const small = tarch::accelerator::lockReductionScratch(64);
      tarch::accelerator::unlockReductionScratch();
      void* const smallAgain = tarch::accelerator::lockReductionScratch(32);
      tarch::accelerator::unlockReductionScratch();
      void* const large = tarch::accelerator::lockReductionScratch(4096);
      tarch::accelerator::unlockReductionScratch();
      void* const largeAgain   = tarch::accelerator::lockReductionScratch(4096);
      void* const staging      = tarch::accelerator::getReductionStaging(16);
      void* const stagingAgain = tarch::accelerator::getReductionStaging(8);
      tarch::accelerator::unlockReductionScratch();

      THEN("Every request returns memory and a request that fits the last allocation returns its address") {
        VALIDATE(small != nullptr);
        VALIDATE(large != nullptr);
        VALIDATE_EQUALS(smallAgain, small);
        VALIDATE_EQUALS(largeAgain, large);
        VALIDATE_EQUALS(stagingAgain, staging);
      }

      AND_WHEN("Host code releases the reduction scratch and locks eight bytes") {
        tarch::accelerator::releaseReductionScratch();
        void* const afterTheRelease = tarch::accelerator::lockReductionScratch(8);
        tarch::accelerator::unlockReductionScratch();

        THEN("The request returns memory") { VALIDATE(afterTheRelease != nullptr); }
      }
    }
  }
}
