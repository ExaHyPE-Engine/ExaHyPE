// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelLaunchTest.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"
#include "tarch/tests/TestMacros.h"

__global__ static void testDimensionsKernel(int* d) {
#if Dimensions == 2
  d[0] = 2;
#elif Dimensions == 3
  d[0] = 3;
#else
#error "Only 2 or 3 dimensions are supported!
#endif
}

exahype2::tests::KernelLaunchTest::KernelLaunchTest():
  TestCase("exahype2::tests::KernelLaunchTest") {}

void exahype2::tests::KernelLaunchTest::run() { testMethod(testDimensions); }

void exahype2::tests::KernelLaunchTest::testDimensions() {
  int  value   = -1;
  int* d_value = nullptr;

  CHECK_CUDA_ERROR(cudaMalloc((void**)&d_value, sizeof(int)));

  // Launch the kernel with one block and one thread
  testDimensionsKernel<<<1, 1>>>(d_value);

  // Copy the result back from the device
  CHECK_CUDA_ERROR(cudaMemcpy(&value, d_value, sizeof(int), cudaMemcpyDeviceToHost));

  // Free allocated memory on the device
  CHECK_CUDA_ERROR(cudaFree(d_value));

  validateEquals(value, Dimensions);
}
