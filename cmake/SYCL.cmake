# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel|IntelLLVM")
  if(PEANO_NVIDIA_GPU)
    target_compile_options(PeanoInterface INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH}>)
    target_link_options(PeanoInterface INTERFACE -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda)
  elseif(PEANO_AMD_GPU)
    target_compile_options(PeanoInterface INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH}>)
    target_link_options(PeanoInterface INTERFACE -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa)
  elseif(PEANO_INTEL_GPU)
    find_package(IntelSYCL REQUIRED)
    target_link_libraries(PeanoInterface INTERFACE IntelSYCL)
  endif()
endif()

target_compile_definitions(PeanoInterface INTERFACE GPUOffloadingSYCL)
