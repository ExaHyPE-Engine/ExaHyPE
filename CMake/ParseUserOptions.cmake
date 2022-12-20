include(Constants)

# Some Toolchains of Peano do not work (even for autoconf) therefore the current toolchain options are disabled for CMake
# set(WITH_TOOLCHAIN "no" CACHE STRING "${TOOLCHAIN_TYPES}")
# set_property(CACHE WITH_TOOLCHAIN PROPERTY STRINGS "${TOOLCHAIN_TYPES}")
set(WITH_MULTITHREADING
	"omp"
	CACHE STRING
	"no=Without parallelism
		cpp=With C++ threads
		omp=With OpenMP
		sycl=With Sycl GPU Support
		tbb=With Intel Thread Building Blocks")
set_property(CACHE WITH_MULTITHREADING PROPERTY STRINGS ${PARALLELISM_TYPES})
string(TOUPPER ${WITH_MULTITHREADING} WITH_MULTITHREADING_UPPER)
set(Shared${WITH_MULTITHREADING_UPPER} 1)

set(WITH_GPU
	"no"
	CACHE STRING
	"no=Without GPU support
		cpp=With C++ GPU suppoprt
		omp=With OpenMP offloading to GPUs
		sycl=With Sycl GPU Support
		hip=With HIPSycl GPU support")
set_property(CACHE WITH_GPU PROPERTY STRINGS ${GPU_SUPPORT_TYPES})
string(TOUPPER ${WITH_GPU} WITH_GPU_UPPER)
set(GPUOffloading${WITH_GPU_UPPER} 1)

# https://www.kitware.com/cmake-and-the-default-build-type/
# If the user does not provide a build type then it is set to none and nothing is used,
# but if the user sets it, it will interfere with our build set(CMAKE_BUILD_TYPE ...)
# Generate the varaible for the build type
# We need to force a name like "All" instead of undefined for the build type, because
# otherwise project_options will set a default build mode "RelWithDebInfo" by
# project_options therefore we set the name All
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
set(CMAKE_BUILD_TYPE "All" CACHE STRING "Build type: ${BUILD_TYPES}")

# Check that user has provided a valid build target
if(NOT ${CMAKE_BUILD_TYPE} IN_LIST BUILD_TYPES AND NOT ${CMAKE_BUILD_TYPE})
	message(
		FATAL_ERROR
		"No valid build type specified, possible values are: ${BUILD_TYPES}. Received value: ${CMAKE_BUILD_TYPE}"
	)
endif()

# Create the list of compile targets depending on the user input
if(CMAKE_BUILD_TYPE STREQUAL "All" OR NOT CMAKE_BUILD_TYPE)
	message(STATUS "Build for following targets: ${BUILD_TYPES}")
	list(APPEND BUILD_MODES ${BUILD_TYPES_LOWER})

	# We should remove All from build modes as it is not a real build type but a placeholder
	list(REMOVE_ITEM BUILD_MODES "all")
	set(CMAKE_BUILD_TYPE "All")
else()
	message(STATUS "Cmake build type specified, build: ${CMAKE_BUILD_TYPE}")
	string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
	list(APPEND BUILD_MODES ${CMAKE_BUILD_TYPE})
endif()

foreach(mode IN LISTS BUILD_MODES)
	if(${mode} STREQUAL "release")
		set(SUFFIX_${mode} "")
	else()
		set(SUFFIX_${mode} "_${mode}")
	endif()
endforeach()

# Choose GPU Architecture from the defined varaibles
set(GPU_ARCH "" CACHE STRING "GPU architecture sm_XX or gfxXXX")
list(APPEND GPU_ARCHS ${GPU_ARCHITECTURE_TYPES})
set_property(CACHE GPU_ARCH PROPERTY STRINGS ${GPU_ARCHITECTURE_TYPES})

# Generate the variable for Dimensions
set(WITH_DIMENSIONS "" CACHE STRING "Possible values are: ${DIMENSION_TYPES}")
set_property(CACHE WITH_DIMENSIONS PROPERTY STRINGS ${DIMENSION_TYPES})

# Check we have a right dimension
if(NOT WITH_DIMENSIONS IN_LIST DIMENSION_TYPES AND NOT WITH_DIMENSIONS STREQUAL "")
	message(FATAL_ERROR
		"Not a valid dimension was specified. When specified, valid values are: "
		"${DIMENSION_TYPES} "
		"Received: ${WITH_DIMENSIONS}")
else()
	# Create list of dimensions
	if(WITH_DIMENSIONS STREQUAL "" OR WITH_DIMENSIONS STREQUAL "2 and 3")
		message(STATUS "Configuring both for 2D and 3D")
		list(APPEND PEANO_DIMENSIONS "2" "3")
	else()
		message(STATUS "Configuring for ${WITH_DIMENSIONS}D")
		list(APPEND PEANO_DIMENSIONS ${WITH_DIMENSIONS})
	endif()
endif()


# Options not related to project_options
option(WITH_MPI "With MPI" OFF)
option(WITH_OTTER "Enable Otter" OFF)

# Options to enable ExaHyPE2, Toolboxes
option(ENABLE_EXAHYPE "Enable ExaHype2" OFF)
option(ENABLE_FINITE_ELEMENTS "Enable finite elements" OFF)
option(ENABLE_MULTIPRECISION "Enable multi-precision" OFF)
option(ENABLE_LOADBALANCING "Enable loadbalancing" OFF)
option(ENABLE_PARTICLES "Enable particles" OFF)
option(ENABLE_BLOCKSTRUCTURED "Enable blockstructured" OFF)
