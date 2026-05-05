// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(WITH_GPU_CUDA)

#include "DevicePropertiesCache.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  int          numberOfDevices = 0;
  cudaStream_t activeStream    = 0;
} // namespace

bool tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(0));
    cudaDeviceProp* prop = &cuda::DevicePropertiesCache::get();
    std::size_t     free = 0, total = 0;
    CHECK_CUDA_ERROR(cudaMemGetInfo(&free, &total));

    logInfo("[" << device << "]: " << prop->name);
    logDebug("  " << prop->major << "." << prop->minor << " CC");
    logDebug("  " << free / (1024 * 1024) << " MB free of " << total / (1024 * 1024) << " MB total global memory");
    logDebug("  " << prop->totalConstMem << " bytes constant memory");
    logDebug("  " << prop->multiProcessorCount << " SMs");
    logDebug("  " << prop->maxBlocksPerMultiProcessor << " blocks per SM");
    logDebug("  " << prop->maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << prop->regsPerMultiprocessor << " registers per SM");
    logDebug("  " << prop->sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << prop->maxThreadsPerBlock << " threads per block");
    logDebug("  " << prop->regsPerBlock << " registers per block");
    logDebug("  " << prop->sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << prop->warpSize << " threads per warp");
    logDebug("  " << prop->asyncEngineCount << " asynchronous engines");
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  CHECK_CUDA_ERROR(cudaDeviceSynchronize());
  CHECK_CUDA_ERROR(cudaDeviceReset());
}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

cudaStream_t tarch::accelerator::deviceStreamCreate() {
  cudaStream_t stream = 0;
  CHECK_CUDA_ERROR(cudaStreamCreate(&stream));
  return stream;
}

void tarch::accelerator::deviceStreamDestroy(cudaStream_t* stream) {
  CHECK_CUDA_ERROR(cudaStreamDestroy(*stream));
  *stream = (cudaStream_t)0;
}

void tarch::accelerator::deviceStreamSync(cudaStream_t stream) { CHECK_CUDA_ERROR(cudaStreamSynchronize(stream)); }

void tarch::accelerator::deviceStreamSet(cudaStream_t stream) { activeStream = stream; }

cudaStream_t tarch::accelerator::deviceStreamGet() { return activeStream; }

#endif // defined(WITH_GPU_CUDA)
