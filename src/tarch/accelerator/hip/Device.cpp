// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(WITH_GPU_HIP)

#include "DevicePropertiesCache.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryAllocationCache.h"
#include "tarch/accelerator/accelerator.h"

#include <unordered_map>

namespace {
  int                                               numberOfDevices = 0;
  thread_local std::unordered_map<int, hipStream_t> activeStreams;
} // namespace

bool tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_HIP_ERROR(hipGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    CHECK_HIP_ERROR(hipSetDevice(device));
    CHECK_HIP_ERROR(hipFree(0));
    hipDeviceProp_t* prop = &hip::DevicePropertiesCache::get();
    std::size_t      free = 0, total = 0;
    CHECK_HIP_ERROR(hipMemGetInfo(&free, &total));

    logInfo("[" << device << "]: " << prop->name);
    logDebug("  " << prop->major << "." << prop->minor << " CC");
    logDebug("  " << free / (1024 * 1024) << " MB free of " << total / (1024 * 1024) << " MB total global memory");
    logDebug("  " << prop->totalConstMem << " bytes constant memory");
    logDebug("  " << prop->multiProcessorCount << " SMs");
    logDebug("  " << prop->maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << prop->regsPerMultiprocessor << " registers per SM");
    logDebug("  " << prop->sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << prop->maxThreadsPerBlock << " threads per block");
    logDebug("  " << prop->regsPerBlock << " registers per block");
    logDebug("  " << prop->sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << prop->warpSize << " threads per warp");
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
  hipStream_t stream = 0;
  CHECK_HIP_ERROR(hipStreamCreate(&stream));
  return stream;
}

void tarch::accelerator::deviceStreamDestroy(hipStream_t* stream) {
  CHECK_HIP_ERROR(hipStreamDestroy(*stream));
  *stream = (hipStream_t)0;
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
  auto it = activeStreams.find(device);
  return it == activeStreams.end() ? static_cast<hipStream_t>(0) : it->second;
}

#endif // defined(WITH_GPU_HIP)
