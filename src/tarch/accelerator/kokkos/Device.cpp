// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Device.h"

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"

#include <Kokkos_Core.hpp>
#include <mpi.h>
#if defined(OPEN_MPI)
#include <mpi-ext.h>
#endif

namespace {
  int numberOfDevices = 0;
} // namespace

bool
tarch::accelerator::Device::init() {
  Kokkos::initialize();
  numberOfDevices = Kokkos::num_devices();

  LOG_DEBUG("-----------------------------------------");
  std::stringstream configuration;
  Kokkos::print_configuration(configuration, true);

  std::string line;
  while (std::getline(configuration, line)) {
    if (not line.empty()) {
      LOG_DEBUG(line);
    }
  }
  LOG_DEBUG("-----------------------------------------");

  return isInitialised();
}

void
tarch::accelerator::Device::shutdown() {
  Kokkos::fence();
  Kokkos::finalize();
  numberOfDevices = 0;
}

bool
tarch::accelerator::Device::isInitialised() const {
  return numberOfDevices > 0;
}

/**
 * Returns the threads the default execution space runs at once.
 */
int
tarch::accelerator::Device::getNumberOfConcurrentThreads() const {
  return std::max(1, static_cast<int>(Kokkos::DefaultExecutionSpace().concurrency()));
}

int
tarch::accelerator::Device::getNumberOfDevices() const {
  return numberOfDevices;
}

/**
 * Returns whether the MPI library sends and receives the buffers of the CUDA or HIP device of
 * Kokkos, or std::nullopt for another device or an MPI library that cannot answer this query.
 */
std::optional<bool>
tarch::accelerator::Device::isMPIGPUAware() const {
#if defined(KOKKOS_ENABLE_CUDA) and defined(OPEN_MPI) and defined(MPIX_CUDA_AWARE_SUPPORT)
  return MPIX_Query_cuda_support() == 1;
#elif defined(KOKKOS_ENABLE_CUDA) and defined(MPIX_GPU_SUPPORT_CUDA)
  int supported = 0;
  MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &supported);
  return supported == 1;
#elif defined(KOKKOS_ENABLE_HIP) and defined(OPEN_MPI) and defined(MPIX_ROCM_AWARE_SUPPORT)
  return MPIX_Query_rocm_support() == 1;
#elif defined(KOKKOS_ENABLE_HIP) and defined(MPIX_GPU_SUPPORT_HIP)
  int supported = 0;
  MPIX_GPU_query_support(MPIX_GPU_SUPPORT_HIP, &supported);
  return supported == 1;
#else
  return std::nullopt;
#endif
}

/**
 * Returns the PCI bus address of the CUDA or HIP device of Kokkos, or an empty string for another
 * device kind. The ranks of one node that use the same GPU return the same address.
 */
std::string
tarch::accelerator::Device::getIdOfCurrentDevice() const {
#if defined(KOKKOS_ENABLE_CUDA)
  std::array<char, 32> busAddress{};
  if (
    cudaDeviceGetPCIBusId(busAddress.data(), static_cast<int>(busAddress.size()), Kokkos::device_id()) != cudaSuccess
  ) {
    tarch::terminateWithError("The CUDA runtime cannot report the PCI bus address of the GPU.");
  }
  return busAddress.data();
#elif defined(KOKKOS_ENABLE_HIP)
  std::array<char, 32> busAddress{};
  if (hipDeviceGetPCIBusId(busAddress.data(), static_cast<int>(busAddress.size()), Kokkos::device_id()) != hipSuccess) {
    tarch::terminateWithError("The HIP runtime cannot report the PCI bus address of the GPU.");
  }
  return busAddress.data();
#else
  return {};
#endif
}

#endif // defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
