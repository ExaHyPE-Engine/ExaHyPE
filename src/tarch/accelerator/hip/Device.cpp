// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/hip/DevicePropertiesCache.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"

namespace {
  int                                               numberOfDevices = 0;
  thread_local std::unordered_map<int, hipStream_t> activeStreams;
} // namespace

bool tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_HIP_ERROR(hipGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipFree(nullptr));
    const hipDeviceProp_t* properties = &hip::DevicePropertiesCache::get();
    std::size_t            freeBytes  = 0;
    std::size_t            totalBytes = 0;
    CHECK_HIP_ERROR(hipMemGetInfo(&freeBytes, &totalBytes));

    logInfo("[" << device << "]: " << properties->name);
    logDebug("  " << properties->major << "." << properties->minor << " CC");
    logDebug(
      "  " << freeBytes / (1024 * 1024) << " MB free of " << totalBytes / (1024 * 1024) << " MB total global memory"
    );
    logDebug("  " << properties->totalConstMem << " bytes constant memory");
    logDebug("  " << properties->multiProcessorCount << " SMs");
    logDebug("  " << properties->maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << properties->regsPerMultiprocessor << " registers per SM");
    logDebug("  " << properties->sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << properties->maxThreadsPerBlock << " threads per block");
    logDebug("  " << properties->regsPerBlock << " registers per block");
    logDebug("  " << properties->sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << properties->warpSize << " threads per warp");
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

void tarch::accelerator::Device::shutdown() {
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipDeviceSynchronize());
  }

  MemoryAllocationCache::getInstance().clear();

  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipDeviceReset());
  }

  activeStreams.clear();
  numberOfDevices = 0;
}

bool tarch::accelerator::Device::isInitialised() const { return numberOfDevices > 0; }

int tarch::accelerator::Device::getNumberOfDevices() const { return numberOfDevices; }

hipStream_t tarch::accelerator::deviceStreamCreate() {
  hipStream_t stream = nullptr;
  CHECK_HIP_ERROR(hipStreamCreate(&stream));
  return stream;
}

void tarch::accelerator::deviceStreamDestroy(hipStream_t* stream) {
  CHECK_HIP_ERROR(hipStreamDestroy(*stream));
  *stream = nullptr;
}

void tarch::accelerator::deviceStreamSync(hipStream_t stream) { CHECK_HIP_ERROR(hipStreamSynchronize(stream)); }

void tarch::accelerator::deviceStreamSet(hipStream_t stream) {
  int device = 0;
  CHECK_HIP_ERROR(hipGetDevice(&device));
  activeStreams[device] = stream;
}

hipStream_t tarch::accelerator::deviceStreamGet() {
  int device = 0;
  CHECK_HIP_ERROR(hipGetDevice(&device));
  const auto activeStream = activeStreams.find(device);
  return activeStream == activeStreams.end() ? nullptr : activeStream->second;
}

#endif // defined(PEANO_WITH_GPU_BACKEND_HIP)
