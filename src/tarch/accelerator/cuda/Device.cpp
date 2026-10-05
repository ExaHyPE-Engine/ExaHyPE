// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/cuda/DevicePropertiesCache.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"

namespace {
  int                                                numberOfDevices = 0;
  thread_local std::unordered_map<int, cudaStream_t> activeStreams;
} // namespace

bool
tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    cudaDeviceProp properties{};
    CHECK_CUDA_ERROR(cudaGetDeviceProperties(&properties, device));

    logInfo("[" << device << "]: " << properties.name);
    logDebug("  " << properties.major << "." << properties.minor << " CC");
    logDebug("  " << properties.totalGlobalMem / (1024 * 1024) << " MB global memory");
    logDebug("  " << properties.totalConstMem << " bytes constant memory");
    logDebug("  " << properties.multiProcessorCount << " SMs");
    logDebug("  " << properties.maxBlocksPerMultiProcessor << " blocks per SM");
    logDebug("  " << properties.maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << properties.regsPerMultiprocessor << " registers per SM");
    logDebug("  " << properties.sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << properties.maxThreadsPerBlock << " threads per block");
    logDebug("  " << properties.regsPerBlock << " registers per block");
    logDebug("  " << properties.sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << properties.warpSize << " threads per warp");
    logDebug("  " << properties.asyncEngineCount << " asynchronous engines");
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

/**
 * Synchronises and resets the device of this rank, the only device the rank holds a context on.
 */
void
tarch::accelerator::Device::shutdown() {
  assertion(
    tarch::accelerator::getCurrentDevice() < numberOfDevices,
    tarch::accelerator::getCurrentDevice(),
    numberOfDevices
  );
  CHECK_CUDA_ERROR(cudaSetDevice(tarch::accelerator::getCurrentDevice()));
  CHECK_CUDA_ERROR(cudaDeviceSynchronize());
  tarch::accelerator::releaseReductionScratch();
  CHECK_CUDA_ERROR(cudaDeviceReset());

  activeStreams.clear();
  numberOfDevices = 0;
}

bool
tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

/**
 * Returns the threads the current device runs at once: Its multiprocessors times the threads
 * one multiprocessor holds.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  const cudaDeviceProp& properties = tarch::accelerator::cuda::DevicePropertiesCache::getProperties();
  return std::max(1, properties.multiProcessorCount * properties.maxThreadsPerMultiProcessor);
}

void
tarch::accelerator::setDeviceStream(cudaStream_t stream) {
  int device = 0;
  CHECK_CUDA_ERROR(cudaGetDevice(&device));
  activeStreams[device] = stream;
}

cudaStream_t
tarch::accelerator::getDeviceStream() {
  int device = 0;
  CHECK_CUDA_ERROR(cudaGetDevice(&device));
  const auto activeStream = activeStreams.find(device);
  return activeStream == activeStreams.end() ? nullptr : activeStream->second;
}

#endif // defined(PEANO_WITH_GPU_BACKEND_CUDA)
