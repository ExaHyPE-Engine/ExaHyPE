# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

if(PEANO_NVIDIA_GPU)
  set(OFFLOADING_FLAGS -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
elseif(PEANO_AMD_GPU)
  set(OFFLOADING_FLAGS -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
elseif(PEANO_INTEL_GPU)
  find_package(IntelSYCL REQUIRED)
  target_link_libraries(PeanoOptions INTERFACE IntelSYCL::SYCL_CXX)
  set(OFFLOADING_FLAGS -fsycl -fsycl-targets=spir64)
  set(ONLY_IF_GPU_FIND_DEPENDENCY "find_dependency(IntelSYCL)")
endif()

if(OFFLOADING_FLAGS)
  target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OFFLOADING_FLAGS}>)
  target_link_options(PeanoOptions INTERFACE ${OFFLOADING_FLAGS})
  target_compile_definitions(PeanoOptions INTERFACE GPUOffloadingSYCL)
  message(STATUS "Using SYCL offloading flags: ${OFFLOADING_FLAGS}")
endif()
