// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/DevicePropertiesCache.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"

namespace {
  int                                                numberOfDevices = 0;
  thread_local std::unordered_map<int, cudaStream_t> activeStreams;
} // namespace

bool tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaFree(nullptr));
    const cudaDeviceProp* properties = &cuda::DevicePropertiesCache::get();
    std::size_t           freeBytes  = 0;
    std::size_t           totalBytes = 0;
    CHECK_CUDA_ERROR(cudaMemGetInfo(&freeBytes, &totalBytes));

    logInfo("[" << device << "]: " << properties->name);
    logDebug("  " << properties->major << "." << properties->minor << " CC");
    logDebug(
      "  " << freeBytes / (1024 * 1024) << " MB free of " << totalBytes / (1024 * 1024) << " MB total global memory"
    );
    logDebug("  " << properties->totalConstMem << " bytes constant memory");
    logDebug("  " << properties->multiProcessorCount << " SMs");
    logDebug("  " << properties->maxBlocksPerMultiProcessor << " blocks per SM");
    logDebug("  " << properties->maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << properties->regsPerMultiprocessor << " registers per SM");
    logDebug("  " << properties->sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << properties->maxThreadsPerBlock << " threads per block");
    logDebug("  " << properties->regsPerBlock << " registers per block");
    logDebug("  " << properties->sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << properties->warpSize << " threads per warp");
    logDebug("  " << properties->asyncEngineCount << " asynchronous engines");
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
  }

  MemoryAllocationCache::getInstance().clear();

  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_CUDA_ERROR(cudaSetDevice(device));
    CHECK_CUDA_ERROR(cudaDeviceReset());
  }

  activeStreams.clear();
  numberOfDevices = 0;
}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

cudaStream_t tarch::accelerator::deviceStreamCreate() {
  cudaStream_t stream = nullptr;
  CHECK_CUDA_ERROR(cudaStreamCreate(&stream));
  return stream;
}

void tarch::accelerator::deviceStreamDestroy(cudaStream_t* stream) {
  CHECK_CUDA_ERROR(cudaStreamDestroy(*stream));
  *stream = nullptr;
}

void tarch::accelerator::deviceStreamSync(cudaStream_t stream) { CHECK_CUDA_ERROR(cudaStreamSynchronize(stream)); }

void tarch::accelerator::deviceStreamSet(cudaStream_t stream) {
  int device = 0;
  CHECK_CUDA_ERROR(cudaGetDevice(&device));
  activeStreams[device] = stream;
}

cudaStream_t tarch::accelerator::deviceStreamGet() {
  int device = 0;
  CHECK_CUDA_ERROR(cudaGetDevice(&device));
  const auto activeStream = activeStreams.find(device);
  return activeStream == activeStreams.end() ? nullptr : activeStream->second;
}

#endif // defined(PEANO_WITH_GPU_BACKEND_CUDA)
