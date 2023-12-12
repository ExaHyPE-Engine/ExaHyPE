# CMAKE_CXX_FLAGS_* are appended to the targets (build modes) automatically in most of the cases,
# CMAKE_CXX_FLAGS_* are appended to every target, regardless of build mode. e.g., Release, Debug
# But an uncommon name such as _TRACE is not always appended (CMAKE_CXX_FLAGS_TRACE).
# Therefore, we will use PEANO_CXX_FLAGS_* variables instead and set CMAKE_CXX_FLAG_* to "".
# Furthermore, CMAKE_CXX_FLAGS is the environment variable when user-defined flags are to be used in the environment.
# Variables CXXFLAGS="..." are added to every target, which we should not touch here.
# This allows for the CMake build system to use exactly the same flags as the Automake build system
# for Peano regardless of the default configuration, unless the user provides flags.
# Release and Debug are also used by Peano, MinSizeRel and RelWithDebInfo are not used by Peano.
# It needs to be done similarly for definitions and LDFLAGS.

# The CMAKE_CXX_FLAGS_*_INIT are used to initialize the flags and should not be used during compilation,
# But with nvc++ and CUDA backend I have noticed that they are used, just in case we remove them too
# as our flags provide the same behaviour as *_INIT flags.

# The compile flags given by the user in the env variable CUDAFLAGS is used for CMAKE_CUDA_FLAGS.
# CUDA initiliases with -O3 and -DNDEBUG by default, we need to override it.
foreach(BUILD_TYPE DEBUG;RELEASE;ASSERTS;TRACE;STATS)
  set(CMAKE_CXX_FLAGS_${BUILD_TYPE} "")
  set(CMAKE_CXX_FLAGS_${BUILD_TYPE}_INIT "")
  set(${BUILD_TYPE}_DEFINITIONS "")
  set(${BUILD_TYPE}_LDFLAGS "")
  set(CUDA_NVCC_FLAGS "") # Not used normally, but in case it is used by any platform
  set(CUDA_NVCC_FLAGS_${BUILD_TYPE} "") # Same as above
  set(CMAKE_CUDA_FLAGS_INIT "")
  set(CMAKE_CUDA_FLAGS_${BUILD_TYPE} "")
  set(CMAKE_CUDA_FLAGS_${BUILD_TYPE}_INIT "")
endforeach()
