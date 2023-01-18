message(STATUS "Configuring ExaHype 2")

file(GLOB_RECURSE EXAHYPE2_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(mode IN LISTS BUILD_MODES)
	string(TOUPPER ${mode} mode_upper)
	foreach(dimension IN LISTS PEANO_DIMENSIONS)
		message(STATUS "Adding the ExaHyPE2 target with dimension=${dimension} and build_mode=${mode}")

		add_library(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} STATIC)

		target_sources(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${EXAHYPE2_SOURCES})
		target_include_directories(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${CMAKE_SOURCE_DIR}/src  ${CMAKE_CURRENT_SOURCE_DIR})
		
		target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE Tarch${SUFFIX_${mode}})
		
		target_compile_definitions(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE Dimensions=${dimension})
		target_compile_options(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_FLAGS})
		target_compile_definitions(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_DEFINITIONS})
		target_link_options(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${${mode_upper}_LDFLAGS})

		if(ENABLE_FINITE_ELEMENTS)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ToolboxFiniteElements${dimension}d${SUFFIX_${mode}})
		endif()

		if(ENABLE_MULTIPRECISION)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ToolboxMultiprecision${dimension}d${SUFFIX_${mode}})
		endif()

		if(ENABLE_LOADBALANCING)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ToolboxLoadBalancing${dimension}d${SUFFIX_${mode}})
		endif()

		if(ENABLE_PARTICLES)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ToolboxParticles${dimension}d${SUFFIX_${mode}})
		endif()

		if(ENABLE_BLOCKSTRUCTURED)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ToolboxBlockStructured${dimension}d${SUFFIX_${mode}})
		endif()

		if(WITH_OTTER)
			target_link_libraries(ExaHyPE2Core${dimension}d${SUFFIX_${mode}} PRIVATE ${Otter_LIBRARIES})
		endif()

	endforeach()
endforeach()
