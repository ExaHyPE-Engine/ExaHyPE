// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelConstructsTest.h"

#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/accelerator.h"
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
 * A teamBarrier call makes the writes of every team member visible to the other members.
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
 * Consecutive reductions inside a team add up on top of the seeded target.
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
 * A custom reduction over a flat range keeps a seeded target that no entry exceeds.
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
