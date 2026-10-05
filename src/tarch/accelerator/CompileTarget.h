// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/Device.h"
#endif

#if defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/Device.h"
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Device.h"
#endif

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h"
#endif

#include "tarch/CompilerSpecificSettings.h"

#define TARGET_DECLARE
#define TARGET_DECLARE_END
#define GPU_CALLABLE_METHOD

#if defined(PEANO_WITH_GPU_BACKEND_SYCL) and defined(ACPP_UNIVERSAL_TARGET)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD ACPP_UNIVERSAL_TARGET
#endif

#if defined(PEANO_WITH_GPU_BACKEND_CUDA) or defined(PEANO_WITH_GPU_BACKEND_HIP)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD __host__ __device__
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD KOKKOS_FUNCTION
#endif

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)
#undef TARGET_DECLARE
#undef TARGET_DECLARE_END
#define TARGET_DECLARE _Pragma("omp declare target")
#define TARGET_DECLARE_END _Pragma("omp end declare target")
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#define COMPILE_IF_ON_DEVICE(CODE) KOKKOS_IF_ON_DEVICE(CODE)
#define COMPILE_IF_ON_HOST(CODE) KOKKOS_IF_ON_HOST(CODE)
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL) and defined(ACPP_UNIVERSAL_TARGET)
#define COMPILE_IF_ON_DEVICE(CODE) __acpp_if_target_device(CODE)
#define COMPILE_IF_ON_HOST(CODE) __acpp_if_target_host(CODE)
#else
#if defined(__CUDA_ARCH__) or defined(__HIP_DEVICE_COMPILE__) or defined(__SPIR__) or defined(__SPIRV__) \
  or defined(__SYCL_DEVICE_ONLY__) or defined(__NVPTX__) or defined(__AMDGCN__)
#define COMPILE_IF_ON_DEVICE(CODE) {CODE}
#define COMPILE_IF_ON_HOST(CODE)
#else
#define COMPILE_IF_ON_DEVICE(CODE)
#define COMPILE_IF_ON_HOST(CODE) {CODE}
#endif
#endif

#if (defined(__CUDA_ARCH__) or defined(__HIP_DEVICE_COMPILE__)) and not defined(PEANO_WITH_GPU_BACKEND_OMPT)
#define DEVICE_VARIABLE __device__
#else
#define DEVICE_VARIABLE
#endif
