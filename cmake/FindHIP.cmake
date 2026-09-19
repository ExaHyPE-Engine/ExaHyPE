# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_package(hip CONFIG REQUIRED)
set(HIP_FOUND "${hip_FOUND}")

if(TARGET PeanoOptions)
  target_link_libraries(PeanoOptions INTERFACE hip::host)
  target_compile_options(PeanoOptions INTERFACE -fPIC)
  target_compile_features(PeanoOptions INTERFACE hip_std_20)
  target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_HIP PEANO_WITH_GPU_BACKEND)
endif()
