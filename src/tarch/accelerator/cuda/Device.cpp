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

#include <mpi.h>
#if defined(OPEN_MPI)
#include <mpi-ext.h>
#endif

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
 * Synchronises and resets the device of this rank. The rank creates a context on this device only.
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
 * Returns the threads the current device runs at once. These are its multiprocessors times the
 * resident threads of one multiprocessor.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  const cudaDeviceProp& properties = tarch::accelerator::cuda::DevicePropertiesCache::getProperties();
  return std::max(1, properties.multiProcessorCount * properties.maxThreadsPerMultiProcessor);
}

/**
 * Returns whether the MPI library sends and receives CUDA device buffers, or std::nullopt for an
 * MPI library that answers no such query.
 */
std::optional<bool>
tarch::accelerator::Device::isMPIGPUAware() const {
#if defined(OPEN_MPI) and defined(MPIX_CUDA_AWARE_SUPPORT)
  return MPIX_Query_cuda_support() == 1;
#elif defined(MPIX_GPU_SUPPORT_CUDA)
  int supported = 0;
  MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &supported);
  return supported == 1;
#else
  return std::nullopt;
#endif
}

/**
 * Returns the PCI bus address of the current device. The ranks of one node that use the same GPU
 * return the same address.
 */
std::string
tarch::accelerator::Device::getIdOfCurrentDevice() const {
  std::array<char, 32> busAddress{};
  CHECK_CUDA_ERROR(cudaDeviceGetPCIBusId(
    busAddress.data(),
    static_cast<int>(busAddress.size()),
    tarch::accelerator::getCurrentDevice()
  ));
  return busAddress.data();
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
