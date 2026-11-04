// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/Device.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int ElementCount    = 1024;
  constexpr int ThreadsPerBlock = 256;

  __global__ void reduceSum(const int* const input, const int count, int* const result) {
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
} // namespace

UNIT_TEST("A reduction kernel sums every element of a device array into one device integer") {
  GIVEN("A device array of 1024 ones and a device result of zero on the current device") {
    const std::vector<int> host(ElementCount, 1);
    const int              zero   = 0;
    const int              device = tarch::accelerator::getCurrentDevice();

    int* const deviceInput  = tarch::allocateMemory<int>(ElementCount, tarch::MemoryLocation::Device, device);
    int* const deviceResult = tarch::allocateMemory<int>(1, tarch::MemoryLocation::Device, device);

    tarch::copyTo(deviceInput, host.data(), ElementCount, tarch::MemoryLocation::Device, device);
    tarch::copyTo(deviceResult, &zero, 1, tarch::MemoryLocation::Device, device);

    WHEN("Blocks of 256 threads reduce the array into the result and the host copies the result") {
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

      THEN("The sum is 1024") { VALIDATE_EQUALS(deviceSum, ElementCount); }
    }
    tarch::freeMemory(deviceInput, tarch::MemoryLocation::Device, device);
    tarch::freeMemory(deviceResult, tarch::MemoryLocation::Device, device);
  }
}
