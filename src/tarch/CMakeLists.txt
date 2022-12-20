message(STATUS "Configuring tarch")

file(GLOB_RECURSE TARCH_SOURCES *.cpp)

if(WITH_MULTITHREADING STREQUAL "omp" AND NOT OpenMP_CXX_FOUND)
	message(FATAL_ERROR "OpenMP configuration requires OpenMP to be present!")
endif()

foreach(mode IN LISTS BUILD_MODES)
	string(TOUPPER ${mode} mode_upper)
	message(STATUS "Adding a tarch target with build_mode=${mode}")

	add_library(Tarch${SUFFIX_${mode}} STATIC)

	if(SharedOMP)
		if(NOT OpenMP_CXX_FOUND)
			message(FATAL_ERROR "OpenMP configuration requires OpenMP to be present!")
		endif()
	endif()

	# Some Toolchains of Peano do not work (even for autoconf) therefore the current toolchain options are disabled for CMake
	# if(WITH_TOOLCHAIN STREQUAL "scorep")
	# # TODO: Fix logging libraries
	# find_library(PAPI libpapi.a papi)
	# find_library(CUBE4W libcube4w.a cube4w)
	# target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${PAPI})
	# target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${CUBE4W})
	# target_link_directories(Tarch${SUFFIX_${mode}} PRIVATE ${LD_LIBRARY_PATH})
	# target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_LIBRARIES})
	# target_compile_options(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_COMPILE_FLAGS})
	# target_link_options(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_LINK_OPTION})
	# elseif(WITH_TOOLCHAIN STREQUAL "intel")
	# target_link_libraries(Tarch${SUFFIX_${mode}} INTERFACE VT::VT)
	# target_include_directories(Tarch${SUFFIX_${mode}} SYSTEM PRIVATE ${VT_INCLUDE_DIRS})
	# target_link_directories(Tarch${SUFFIX_${mode}} INTERFACE ${VT_LIBRARIES_DIR})
	# endif()

	# TODO: Support other parallelization methods too
	if(WITH_MULTITHREADING STREQUAL "omp")
		if(WITH_GPU STREQUAL "omp")
			target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC OpenMP::OpenMP_CXX_with_offloading)
		else()
			target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC OpenMP::OpenMP_CXX)
		endif()
	elseif(WITH_MULTITHREADING STREQUAL "cpp")
		target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC Threads::Threads)
	elseif(WITH_MULTITHREADING STREQUAL "tbb")
		target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC TBB::tbb)
	endif()

	if(WITH_MPI)
		target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC MPI::MPI_CXX)
	endif()

	if(WITH_OTTER)
		target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC Otter::Otter)
	endif()

	target_sources(Tarch${SUFFIX_${mode}} PRIVATE ${TARCH_SOURCES})
	target_include_directories(Tarch${SUFFIX_${mode}} PRIVATE ${CMAKE_SOURCE_DIR}/src)
	target_link_directories(Tarch${SUFFIX_${mode}} PRIVATE ${CMAKE_SOURCE_DIR}/src)
	target_compile_options(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_FLAGS})
	target_compile_definitions(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_DEFINITIONS})
	target_link_options(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_LDFLAGS})

endforeach()
