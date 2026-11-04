// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionTest.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/Device.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int ElementCount    = 1024;
  constexpr int ThreadsPerBlock = 256;
} // namespace

__global__ static void
reduceSum(const int* input, int count, int* result) {
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

tarch::accelerator::cuda::tests::ReductionTest::ReductionTest() :
  tarch::tests::TestCase("tarch::accelerator::cuda::tests::ReductionTest") {}

void
tarch::accelerator::cuda::tests::ReductionTest::run() {
  testMethod(testParallelReduction);
}

void
tarch::accelerator::cuda::tests::ReductionTest::testParallelReduction() {
  const std::vector<int> host(ElementCount, 1);
  const int              expectedSum = ElementCount;
  const int              zero        = 0;
  const int              device      = tarch::accelerator::getCurrentDevice();

  int* deviceInput  = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
  int* deviceResult = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, device);

  tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);
  tarch::copyTo(deviceResult, &zero, 1, tarch::MemoryLocation::Device, device);

  const int numberOfBlocks = (ElementCount + ThreadsPerBlock - 1) / ThreadsPerBlock;
  CHECK_CUDA_ERROR(cudaSetDevice(device));
  reduceSum<<<numberOfBlocks, ThreadsPerBlock, 0, tarch::accelerator::getDeviceStream()>>>(
    deviceInput,
    ElementCount,
    deviceResult
  );
  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::getDeviceStream()));

  int deviceSum = -1;
  tarch::copyFrom(deviceResult, &deviceSum, 1, tarch::MemoryLocation::Device, device);

  tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);
  tarch::freeMemory(deviceResult, tarch::MemoryLocation::Device, device);

  validateEqualsWithParams1(deviceSum, expectedSum, deviceSum);
}
