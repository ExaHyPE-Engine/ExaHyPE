// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/hip/Device.h"
#include "tarch/accelerator/hip/ErrorCheck.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int DeviceId        = tarch::accelerator::Device::DefaultDevice;
  constexpr int ElementCount    = 1024;
  constexpr int ThreadsPerBlock = 256;
} // namespace

__global__ static void reduceSum(const int* input, int count, int* result) {
  __shared__ int partial[ThreadsPerBlock];

  const int localIndex  = threadIdx.x;
  const int globalIndex = blockIdx.x * blockDim.x + localIndex;

  partial[localIndex] = (globalIndex < count) ? input[globalIndex] : 0;
  __syncthreads();

  for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
    if (localIndex < stride) {
      partial[localIndex] += partial[localIndex + stride];
    }
    __syncthreads();
  }

  if (localIndex == 0) {
    atomicAdd(result, partial[0]);
  }
}

tarch::accelerator::hip::tests::ReductionTest::ReductionTest() :
  TestCase("tarch::accelerator::hip::tests::ReductionTest") {}

void tarch::accelerator::hip::tests::ReductionTest::run() { testMethod(testParallelReduction); }

void tarch::accelerator::hip::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;
  const int              zero        = 0;

  int* deviceInput  = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, DeviceId);
  int* deviceResult = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, DeviceId);

  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, DeviceId);
  tarch::copyTo(deviceResult, &zero, 1, tarch::MemoryLocation::Device, DeviceId);

  const int numberOfBlocks = (ElementCount + ThreadsPerBlock - 1) / ThreadsPerBlock;
  CHECK_HIP_ERROR(hipSetDevice(DeviceId));
  reduceSum<<<numberOfBlocks, ThreadsPerBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    deviceInput,
    ElementCount,
    deviceResult
  );
  CHECK_HIP_ERROR(hipStreamSynchronize(tarch::accelerator::deviceStreamGet()));

  int deviceSum = -1;
  tarch::copyFrom(deviceResult, &deviceSum, 1, tarch::MemoryLocation::Device, DeviceId);

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, DeviceId);
  tarch::freeMemory(deviceResult, tarch::MemoryLocation::Device, DeviceId);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
