# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

find_package(IntelSYCL REQUIRED)
target_link_libraries(PeanoOptions INTERFACE IntelSYCL::SYCL_CXX)

set(OFFLOADING_FLAGS -fsycl -fsycl-targets=spir64)
set(GPU_FIND_DEPENDENCY "find_dependency(IntelSYCL)" CACHE INTERNAL "")

target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OFFLOADING_FLAGS}>)
target_link_options(PeanoOptions INTERFACE ${OFFLOADING_FLAGS})
message(STATUS "Using SYCL offloading flags: ${OFFLOADING_FLAGS}")

target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_SYCL WITH_GPU)
