// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"

namespace {
  int                                               numberOfDevices = 0;
  thread_local std::unordered_map<int, hipStream_t> activeStreams;
} // namespace

bool
tarch::accelerator::Device::init() {
  logDebug("-----------------------------------------");
  CHECK_HIP_ERROR(hipGetDeviceCount(&numberOfDevices));
  for (int device = 0; device < numberOfDevices; device++) {
    hipDeviceProp_t properties{};
    CHECK_HIP_ERROR(hipGetDeviceProperties(&properties, device));

    logInfo("[" << device << "]: " << properties.name);
    logDebug("  " << properties.major << "." << properties.minor << " CC");
    logDebug("  " << properties.totalGlobalMem / (1024 * 1024) << " MB global memory");
    logDebug("  " << properties.totalConstMem << " bytes constant memory");
    logDebug("  " << properties.multiProcessorCount << " SMs");
    logDebug("  " << properties.maxThreadsPerMultiProcessor << " threads per SM");
    logDebug("  " << properties.regsPerMultiprocessor << " registers per SM");
    logDebug("  " << properties.sharedMemPerMultiprocessor << " bytes shared memory per SM");
    logDebug("  " << properties.maxThreadsPerBlock << " threads per block");
    logDebug("  " << properties.regsPerBlock << " registers per block");
    logDebug("  " << properties.sharedMemPerBlock << " bytes shared memory per block");
    logDebug("  " << properties.warpSize << " threads per warp");
  }
  logDebug("-----------------------------------------");

  return isInitialised();
}

/** Synchronises and resets the device of this rank, the only device the rank holds a context on. */
void
tarch::accelerator::Device::shutdown() {
  assertion(
    tarch::accelerator::getCurrentDevice() < numberOfDevices,
    tarch::accelerator::getCurrentDevice(),
    numberOfDevices
  );
  CHECK_HIP_ERROR(hipSetDevice(tarch::accelerator::getCurrentDevice()));
  CHECK_HIP_ERROR(hipDeviceSynchronize());
  CHECK_HIP_ERROR(hipDeviceReset());

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

hipStream_t
tarch::accelerator::deviceStreamCreate() {
  hipStream_t stream = nullptr;
  CHECK_HIP_ERROR(hipStreamCreate(&stream));
  return stream;
}

void
tarch::accelerator::deviceStreamDestroy(hipStream_t* stream) {
  CHECK_HIP_ERROR(hipStreamDestroy(*stream));
  *stream = nullptr;
}

void
tarch::accelerator::deviceStreamSync(hipStream_t stream) {
  CHECK_HIP_ERROR(hipStreamSynchronize(stream));
}

void
tarch::accelerator::setDeviceStream(hipStream_t stream) {
  int device = 0;
  CHECK_HIP_ERROR(hipGetDevice(&device));
  activeStreams[device] = stream;
}

hipStream_t
tarch::accelerator::getDeviceStream() {
  int device = 0;
  CHECK_HIP_ERROR(hipGetDevice(&device));
  const auto activeStream = activeStreams.find(device);
  return activeStream == activeStreams.end() ? nullptr : activeStream->second;
}

#endif // defined(PEANO_WITH_GPU_BACKEND_HIP)
