// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelConstructsTest.h"

#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int N        = 64;
  constexpr int DeviceId = tarch::accelerator::Device::DefaultDevice;
} // namespace

tarch::accelerator::tests::ParallelConstructsTest::ParallelConstructsTest() :
  TestCase("tarch::accelerator::tests::ParallelConstructsTest") {}

void tarch::accelerator::tests::ParallelConstructsTest::run() {
  testMethod(testParallelFor);
  testMethod(testParallelForRangeMD2);
  testMethod(testParallelForRangeMD3);
  testMethod(testParallelForTeam);
  testMethod(testParallelReduceSum);
  testMethod(testParallelReduceMin);
  testMethod(testParallelReduceMax);
  testMethod(testParallelReduceRangeMD2);
  testMethod(testParallelReduceRangeMD3);
  testMethod(testParallelReduceTeam);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelFor() {
  auto* data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(tarch::accelerator::Range(N), [=] GPU_CALLABLE_LAMBDA(const int i) {
    data[i] = static_cast<double>(2 * i);
  });

  tarch::copyFrom(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < N; ++i) {
    correct = correct and (host[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD2() {
  constexpr int Nx   = 8;
  constexpr int Ny   = 8;
  auto*         data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::RangeMD<2>(Nx, Ny),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y) {
      const int i = x * Ny + y;
      data[i]     = static_cast<double>(2 * i);
    }
  );

  tarch::copyFrom(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < N; ++i) {
    correct = correct and (host[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForRangeMD3() {
  constexpr int Nx   = 4;
  constexpr int Ny   = 4;
  constexpr int Nz   = 4;
  auto*         data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::RangeMD<3>(Nx, Ny, Nz),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, const int z) {
      const int i = (x * Ny + y) * Nz + z;
      data[i]     = static_cast<double>(2 * i);
    }
  );

  tarch::copyFrom(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < N; ++i) {
    correct = correct and (host[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelForTeam() {
  constexpr int NumTeams = 8;
  constexpr int TeamSize = 8;
  auto*         data     = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle) {
      tarch::accelerator::parallelFor(teamHandle, tarch::accelerator::Range(TeamSize), [=](const int i) {
        const int idx = team * TeamSize + i;
        data[idx]     = static_cast<double>(2 * idx);
      });
    }
  );

  tarch::copyFrom(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);

  bool correct = true;
  for (int i = 0; i < N; ++i) {
    correct = correct and (host[i] == static_cast<double>(2 * i));
  }
  validate(correct);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceSum() {
  auto* data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = 1.0;
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(N),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& acc) { acc += data[i]; },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(N), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD2() {
  constexpr int Nx   = 8;
  constexpr int Ny   = 8;
  auto*         data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = 1.0;
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<2>(Nx, Ny),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, double& acc) { acc += data[x * Ny + y]; },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(N), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceRangeMD3() {
  constexpr int Nx   = 4;
  constexpr int Ny   = 4;
  constexpr int Nz   = 4;
  auto*         data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = 1.0;
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::RangeMD<3>(Nx, Ny, Nz),
    [=] GPU_CALLABLE_LAMBDA(const int x, const int y, const int z, double& acc) { acc += data[(x * Ny + y) * Nz + z]; },
    tarch::accelerator::Sum<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(N), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceTeam() {
  constexpr int NumTeams = 8;
  constexpr int TeamSize = 8;
  auto*         data     = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(NumTeams),
    [=] GPU_CALLABLE_LAMBDA(const int team, const tarch::accelerator::TeamHandle teamHandle, double& outerAcc) {
      tarch::accelerator::parallelReduce(
        teamHandle,
        tarch::accelerator::Range(TeamSize),
        [=](const int i, double& acc) {
          const double v = data[team * TeamSize + i];
          acc            = (v > acc) ? v : acc;
        },
        tarch::accelerator::Max<double>{outerAcc}
      );
    },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(N), result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMin() {
  auto* data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::max();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(N),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& acc) { acc = (data[i] < acc) ? data[i] : acc; },
    tarch::accelerator::Min<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == 1.0, result);
}

void tarch::accelerator::tests::ParallelConstructsTest::testParallelReduceMax() {
  auto* data = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Device, DeviceId);

  std::array<double, N> host{};
  for (int i = 0; i < N; ++i) {
    host[i] = static_cast<double>(i + 1);
  }
  tarch::copyTo(data, host.data(), N, tarch::MemoryLocation::Device, DeviceId);

  double result = std::numeric_limits<double>::lowest();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(N),
    [=] GPU_CALLABLE_LAMBDA(const int i, double& acc) { acc = (data[i] > acc) ? data[i] : acc; },
    tarch::accelerator::Max<double>{result}
  );

  tarch::freeMemory(data, tarch::MemoryLocation::Device, DeviceId);
  validateWithParams1(result == static_cast<double>(N), result);
}
