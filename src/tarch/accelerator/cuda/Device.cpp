// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(GPUOffloadingCUDA)

#include "ErrorCheck.h"
#include "DevicePropertiesCache.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Lock.h"

namespace {
  int numberOfDevices = 0;
  cudaStream_t activeStream = 0;
} // namespace

tarch::accelerator::Device::Device() {
  CHECK_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
}

void tarch::accelerator::Device::configure(const std::set<int>& devicesToUse) {
  logInfo("configure()", "-----------------------------------------");
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(0));
    cudaDeviceProp* prop = &cuda::DevicePropertiesCache::get();
    std::size_t free = 0, total = 0;
    CHECK_CUDA_ERROR(cudaMemGetInfo(&free, &total));

    constexpr int MB = 1024 * 1024;
    logInfo("configure()", "[" << device << "]: " << prop->name);
    logInfo("configure()", "  " << prop->major << "." << prop->minor << " CC");
    logInfo("configure()", "  " << free / MB << " MB free of " << total / MB << " MB total global memory");
    logInfo("configure()", "  " << prop->totalConstMem << " bytes constant memory");
    logInfo("configure()", "  " << prop->multiProcessorCount << " SMs");
    logInfo("configure()", "  " << prop->maxBlocksPerMultiProcessor << " blocks per SM");
    logInfo("configure()", "  " << prop->maxThreadsPerMultiProcessor << " threads per SM");
    logInfo("configure()", "  " << prop->regsPerMultiprocessor << " registers per SM");
    logInfo("configure()", "  " << prop->sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logInfo("configure()", "  " << prop->maxThreadsPerBlock << " threads per block");
    logInfo("configure()", "  " << prop->regsPerBlock << " registers per block");
    logInfo("configure()", "  " << prop->sharedMemPerBlock << " bytes shared memory per block");
    logInfo("configure()", "  " << prop->warpSize << " threads per warp");
    logInfo("configure()", "  " << prop->asyncEngineCount << " asynchronous engines");
  }
  logInfo("configure()", "-----------------------------------------");
}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  CHECK_CUDA_ERROR(cudaDeviceSynchronize());
  CHECK_CUDA_ERROR(cudaDeviceReset());
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
}

bool tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

cudaStream_t tarch::accelerator::deviceStreamCreate() {
  cudaStream_t stream = 0;
  CHECK_CUDA_ERROR(cudaStreamCreate(&stream));
  return stream;
}

void tarch::accelerator::deviceStreamDestroy(cudaStream_t* stream) {
  CHECK_CUDA_ERROR(cudaStreamDestroy(*stream));
  *stream = (cudaStream_t)0;
}

void tarch::accelerator::deviceStreamSync(cudaStream_t stream) {
  CHECK_CUDA_ERROR(cudaStreamSynchronize(stream));
}

void tarch::accelerator::deviceStreamSet(cudaStream_t stream) {
  activeStream = stream;
}

cudaStream_t tarch::accelerator::deviceStreamGet() {
  return activeStream;
}

#endif // defined(GPUOffloadingCUDA)
