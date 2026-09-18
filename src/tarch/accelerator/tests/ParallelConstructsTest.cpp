// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelConstructsTest.h"

#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int NumberOfEntries = 64;
  constexpr int DeviceId        = tarch::accelerator::Device::DefaultDevice;

  TARGET_DECLARE
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
  TARGET_DECLARE_END
} // namespace

tarch::accelerator::tests::ParallelConstructsTest::ParallelConstructsTest() :
  TestCase("tarch::accelerator::tests::ParallelConstructsTest") {}

void tarch::accelerator::tests::ParallelConstructsTest::run() {
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

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForOnHost() {
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

void tarch::accelerator::tests::ParallelConstructsTest::testParallelFor() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(tarch::accelerator::Range(NumberOfEntries), [=] GPU_CALLABLE_LAMBDA(const int i) {
    data[i] = static_cast<double>(2 * i);
  });

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD2() {
  constexpr int ExtentX = 8;
  constexpr int ExtentY = 8;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y) {
      const int index = x * ExtentY + y;
      data[index]     = static_cast<double>(2 * index);
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD3() {
  constexpr int ExtentX = 4;
  constexpr int ExtentY = 4;
  constexpr int ExtentZ = 4;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, const int z) {
      const int index = (x * ExtentY + y) * ExtentZ + z;
      data[index]     = static_cast<double>(2 * index);
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeam() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle) {
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(TeamSize), [=](const int i) {
        const int index = team * TeamSize + i;
        data[index]     = static_cast<double>(2 * index);
      });
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeamRangeMD2() {
  constexpr int NumberOfTeams = 4;
  constexpr int ExtentX       = 4;
  constexpr int ExtentY       = 4;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle) {
      tarch::accelerator::parallelFor(
        teamHandle,
        tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
        [=](const int x, const int y) {
          const int index = team * ExtentX * ExtentY + x * ExtentY + y;
          data[index]     = static_cast<double>(2 * index);
        }
      );
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeamRangeMD3() {
  constexpr int NumberOfTeams = 8;
  constexpr int ExtentX       = 2;
  constexpr int ExtentY       = 2;
  constexpr int ExtentZ       = 2;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle) {
      tarch::accelerator::parallelFor(
        teamHandle,
        tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
        [=](const int x, const int y, const int z) {
          const int index = team * ExtentX * ExtentY * ExtentZ + (x * ExtentY + y) * ExtentZ + z;
          data[index]     = static_cast<double>(2 * index);
        }
      );
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < NumberOfEntries; ++i) {
    correct = correct and (hostValues[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

/**
 * Checks that teamBarrier makes writes visible to the other team members.
 */
void tarch::accelerator::tests::ParallelConstructsTest::testTeamBarrier() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   scratch       = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  tarch::copyTo(scratch, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle) {
      const int base = team * TeamSize;
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(TeamSize), [=](const int i) {
        scratch[base + i] = static_cast<double>(base + i);
      });
      tarch::accelerator::teamBarrier(teamHandle);
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(TeamSize), [=](const int i) {
        data[base + i] = 2.0 * scratch[base + TeamSize - 1 - i];
      });
    }
  );

  tarch::copyFrom(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(scratch, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int team = 0; team < NumberOfTeams; ++team) {
    for (int i = 0; i < TeamSize; ++i) {
      const double expected = 2.0 * static_cast<double>(team * TeamSize + TeamSize - 1 - i);
      correct               = correct and (hostValues[team * TeamSize + i] == expected);
    }
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceSum() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& accumulator) { accumulator += data[i]; },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD2() {
  constexpr int ExtentX = 8;
  constexpr int ExtentY = 8;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, double& accumulator) { accumulator += data[x * ExtentY + y]; },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD3() {
  constexpr int ExtentX = 4;
  constexpr int ExtentY = 4;
  constexpr int ExtentZ = 4;
  auto* const   data    = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = 1.0;
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, const int z, double& accumulator) {
      accumulator += data[(x * ExtentY + y) * ExtentZ + z];
    },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeam() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle, double& teamAccumulator) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(TeamSize),
        [=](const int i, double& accumulator) {
          const double value = data[team * TeamSize + i];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

/**
 * Checks seeded sums across consecutive team reductions.
 */
void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamSum() {
  constexpr int    NumberOfTeams = 8;
  constexpr int    TeamSize      = 8;
  constexpr double InitialResult = 17.0;
  double           result        = InitialResult;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle, double& teamAccumulator) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(TeamSize),
        [=](const int i, double& accumulator) { accumulator += team * TeamSize + i + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<2>(2, 4),
        [=](const int x, const int y, double& accumulator) { accumulator += team * TeamSize + x * 4 + y + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<3>(2, 2, 2),
        [=](const int x, const int y, const int z, double& accumulator) {
          accumulator += team * TeamSize + (x * 2 + y) * 2 + z + 1;
        },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(0),
        [](const int i, double& accumulator) { accumulator += i + 1; },
        tarch::accelerator::Sum<double>{teamAccumulator}
      );
    },
    tarch::accelerator::Sum<double>{result}
  );
  constexpr int Entries = NumberOfTeams * TeamSize;
  validateWithParams1(result == InitialResult + 3.0 * Entries * (Entries + 1) / 2.0, result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamRangeMD2() {
  constexpr int NumberOfTeams = 4;
  constexpr int ExtentX       = 4;
  constexpr int ExtentY       = 4;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle, double& teamAccumulator) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<2>(ExtentX, ExtentY),
        [=](const int x, const int y, double& accumulator) {
          const double value = data[team * ExtentX * ExtentY + x * ExtentY + y];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeamRangeMD3() {
  constexpr int NumberOfTeams = 8;
  constexpr int ExtentX       = 2;
  constexpr int ExtentY       = 2;
  constexpr int ExtentZ       = 2;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle, double& teamAccumulator) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::RangeMD<3>(ExtentX, ExtentY, ExtentZ),
        [=](const int x, const int y, const int z, double& accumulator) {
          const double value = data[team * ExtentX * ExtentY * ExtentZ + (x * ExtentY + y) * ExtentZ + z];
          accumulator        = (value > accumulator) ? value : accumulator;
        },
        tarch::accelerator::Max<double>{teamAccumulator}
      );
    },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceCustom() {
  constexpr int NumberOfTeams = 8;
  constexpr int TeamSize      = 8;
  auto* const   data          = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  TwoMaxima result{0.0, 0.0};
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumberOfTeams),
    [=] GPU_CALLABLE_LAMBDA(
      const int                            team,
      const tarch::accelerator::TeamHandle teamHandle,
      TwoMaxima&                           teamAccumulator
    ) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(TeamSize),
        [=](const int i, TwoMaxima& accumulator) {
          const double value         = data[team * TeamSize + i];
          const double mirroredValue = 2.0 * NumberOfEntries - value;
          accumulator.first          = (value > accumulator.first) ? value : accumulator.first;
          accumulator.second         = (mirroredValue > accumulator.second) ? mirroredValue : accumulator.second;
        },
        tarch::accelerator::Custom<TwoMaximaOp>{teamAccumulator}
      );
    },
    tarch::accelerator::Custom<TwoMaximaOp>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams2(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
  validateWithParams2(result.second == static_cast<double>(2 * NumberOfEntries - 1), result.first, result.second);
}

/**
 * Checks a custom reduction over a flat range, seeded with a target the reduction has to keep.
 */
void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceCustomRange() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  constexpr double SeededSecond = 3.0 * NumberOfEntries;
  TwoMaxima        result{0.0, SeededSecond};
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    [=] GPU_CALLABLE_LAMBDA(const int i, TwoMaxima& accumulator) {
      const double value         = data[i];
      const double mirroredValue = 2.0 * NumberOfEntries - value;
      accumulator.first          = (value > accumulator.first) ? value : accumulator.first;
      accumulator.second         = (mirroredValue > accumulator.second) ? mirroredValue : accumulator.second;
    },
    tarch::accelerator::Custom<TwoMaximaOp>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams2(result.first == static_cast<double>(NumberOfEntries), result.first, result.second);
  validateWithParams2(result.second == SeededSecond, result.first, result.second);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMin() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::max();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& accumulator) {
      accumulator = (data[i] < accumulator) ? data[i] : accumulator;
    },
    tarch::accelerator::Min<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == 1.0, result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMax() {
  auto* const data = tarch::allocateMemory<double>(NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, NumberOfEntries> hostValues{};
  for (int i = 0; i < NumberOfEntries; ++i) {
    hostValues[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, hostValues.data(), NumberOfEntries, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(NumberOfEntries),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& accumulator) {
      accumulator = (data[i] > accumulator) ? data[i] : accumulator;
    },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(NumberOfEntries), result);
}
