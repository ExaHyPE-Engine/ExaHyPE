# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_package(IntelSYCL REQUIRED)
set(PeanoSYCL_FOUND "${IntelSYCL_FOUND}")

if(TARGET PeanoOptions)
  set(_peano_sycl_flags -fsycl -fsycl-targets=spir64)
  target_link_libraries(PeanoOptions INTERFACE IntelSYCL::SYCL_CXX)
  target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${_peano_sycl_flags}>)
  target_link_options(PeanoOptions INTERFACE ${_peano_sycl_flags})
  target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_SYCL PEANO_WITH_GPU_BACKEND)
  message(STATUS "Using SYCL offloading flags: ${_peano_sycl_flags}")
endif()
