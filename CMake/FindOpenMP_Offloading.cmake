include(FindPackageHandleStandardArgs)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

cmake_push_check_state()

if(NOT GPU_ARCH IN_LIST GPU_ARCHS)
    message(FATAL_ERROR "GPU architechture must be one of: "
                        "${GPU_ARCHS}"
                        "e.g.: -DWITH_GPU=sm_86")
endif()

if (NOT OpenMP_FOUND)
    message(FATAL_ERROR "OpenMP is not found. Make sure that you called find_package(OpenMP REQUIRED) first")
endif()

if (NOT DEFINED GPU_ARCH OR GPU_ARCH STREQUAL "None")
    message(FATAL_ERROR "GPU Arch is not defined")
endif()

if (AMD_GPU AND ${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    message(FATAL_ERROR "Nvidia Compilers don't support AMD GPUs")
endif()


# Global compiler and link options are enable compiling generated targets
# Also see https://lumi-supercomputer.eu/offloading-code-with-compiler-directives/
# for a table of flags see there
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (NVIDIA_GPU)
        # TODO: Ask this
        find_package(CUDA)
        if (NOT CUDA_FOUND)
            message(FATAL_ERROR "OpenMP Offloading Required Cuda libraries on Nvidia GPUs")
        endif()
        set(_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=nvptx64-nvidia-cuda;-Xopenmp-target;-march=${GPU_ARCH}")
        set(_Offloading_LINK_LIBS "-lomptarget")
    else()
        set(_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=amdgcn-amd-amdhsa;-Xopenmp-target;-march=${GPU_ARCH}")
        set(_Offloading_LINK_LIBS "-lomptarget")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 12)
        message(FATAL_ERROR "For GPU Suport with GCC Compilers we need at least version 12")
    endif()
    if (NVIDIA_GPU)
        set(_Offloading_CXX_FLAGS "-fopenmp;-foffload-options=nvptx-none=-march=${GPU_ARCH}")
        set(_Offloading_LINK_LIBS "-lgomp")
    else()
        message(FATAL_ERROR ${NVIDIA_GPU})
        set(_Offloading_CXX_FLAGS "-fopenmp;-foffload-options==amdgcn-amdhs=-march=${GPU_ARCH}")
        set(_Offloading_LINK_LIBS "-lgomp")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    # TODO: Ask this
    if(NOT NVIDIA_GPU)
        message(FATAL_ERROR "Nvidia compilers don't support AMD GPUs (Only Nvidia GPUs supported)")
    endif()
    string(REPLACE "sm_" "cc" OMP_DEVICE_ARCH "${GPU_ARCH}")
    set(_Offloading_CXX_FLAGS "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}")
    set(_Offloading_LINK_LIBS "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "IntelLLVM")
    set(_Offloading_CXX_FLAG "-fiopenmp;-fopenmp-targets=spir64")
    set(_Offloading_LINK_LIBS "-fiopenmp")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    message(FATAL_ERROR "GPU Offloading with CMake does not support legacy Intel compilers. "
                        "Please you use the newer IntelLLVM Compilers or, "
                        "Try it again with the Autoconf build system.")
else()
    message(FATAL_ERROR "GPU Offloading with CMake does not support your compiler: ${CMAKE_CXX_COMPILER_ID}. "
                        "Please you use a compiler that is supported or "
                        "Try it again with the Autoconf build system.")
endif()

# Unlike the other options, CMAKE_REQUIRED_FLAGS should not be ;-separated list
set(CMAKE_REQUIRED_FLAGS "${OpenMP_CXX_FLAGS};${_Offloading_CXX_FLAGS}")
string (REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_INCLUDES "${OpenMP_CXX_INCLUDE_DIRS}")
set(CMAKE_REQUIRED_LIBRARIES "${OpenMP_CXX_LIBRARIES};${_Offloading_LINK_LIBS}")
set(CMAKE_REQUIRED_LINK_OPTIONS "${OpenMP_CXX_LINK_OPTIONS}")
message(STATUS "${CMAKE_REQUIRED_FLAGS}")

check_cxx_source_runs("
    #include <omp.h>
    #define YES 0
    #define NO 1
    int main(int argc, char* argv[]) {
        int canOffload = NO;
        #pragma omp target map(tofrom: canOffload)
        {
            if (!omp_is_initial_device()) {
                canOffload = YES;
            }
        }
        return canOffload;
    }
    " _OPENMP_OFFLOAD_RESULT
)

if (_OPENMP_OFFLOAD_RESULT)
    add_library(OpenMP::OpenMP_CXX_with_offloading INTERFACE IMPORTED)

    set(OpenMP_Offloading_CXX_FLAGS ${_Offloading_TEST_CXX_FLAGS})
    set_property(TARGET OpenMP::OpenMP_CXX_with_offloading APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
            $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_Offloading_CXX_FLAGS}>)

    if (${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
        target_link_options(OpenMP::OpenMP_CXX_with_offloading INTERFACE "-mp=gpu")
    else()
        set(OpenMP_Offloading_LINK_FLAGS ${OpenMP_Offloading_CXX_FLAGS})
        set(OpenMP_Offloading_LINK_LIBRARIES ${_Offloading_LINK_LIBS})
        target_link_libraries(OpenMP::OpenMP_CXX_with_offloading
            INTERFACE
                ${OpenMP_CXX_LIBRARIES}
                ${OpenMP_Offloading_LINK_FLAGS}
                ${OpenMP_Offloading_LINK_LIBRARIES})
    endif()
else()
    if (WITH_GPU STREQUAL "omp")
        message(FATAL_ERROR "OpenMP GPU Offloading was demanded but it was not found."
            "Check your compiler whether it can perform OpenMP GPU offloading.")
    endif()
endif()

add_compile_options("${_Offloading_CXX_FLAGS}")
add_link_options("${_Offloading_LINK_LIBS}")

find_package_handle_standard_args(OpenMP_Offloading REQUIRED_VARS _OPENMP_OFFLOAD_RESULT)
mark_as_advanced(_Offloading_CXX_FLAGS _Offloading_LINK_LIBS)
cmake_pop_check_state()
